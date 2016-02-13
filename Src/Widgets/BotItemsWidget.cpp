
#include "stdafx.h"

BotItemsWidget::BotItemsWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager, DataService& dataService) :
  QWidget(&tabFramework), entityManager(entityManager), dataService(dataService), assetsModel(entityManager)
{
  entityManager.registerListener<EConnection>(*this);
  entityManager.registerListener<EUserSession>(*this);

  setWindowTitle(tr("Bot Assets"));

  QToolBar* toolBar = new QToolBar(this);
  toolBar->setStyleSheet("QToolBar { border: 0px }");
  toolBar->setIconSize(QSize(16, 16));
  toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

  buyAction = toolBar->addAction(QIcon(":/Icons/bitcoin_add.png"), tr("&Buy"));
  buyAction->setEnabled(false);
  buyAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_B));
  connect(buyAction, SIGNAL(triggered()), this, SLOT(newBuyItem()));
  sellAction = toolBar->addAction(QIcon(":/Icons/money_add.png"), tr("&Sell"));
  sellAction->setEnabled(false);
  sellAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_S));
  connect(sellAction, SIGNAL(triggered()), this, SLOT(newSellItem()));

  submitAction = toolBar->addAction(QIcon(":/Icons/bullet_go.png"), tr("S&ubmit"));
  submitAction->setEnabled(false);
  submitAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_U));
  connect(submitAction, SIGNAL(triggered()), this, SLOT(submitItem()));

  cancelAction = toolBar->addAction(QIcon(":/Icons/cancel2.png"), tr("&Cancel"));
  cancelAction->setEnabled(false);
  cancelAction->setShortcut(QKeySequence(Qt::Key_Delete));
  connect(cancelAction, SIGNAL(triggered()), this, SLOT(cancelItem()));

  itemView = new QTreeView(this);
  itemView->setUniformRowHeights(true);
  proxyModel = new UserSessionAssetsSortProxyModel(this);
  proxyModel->setDynamicSortFilter(true);
  proxyModel->setSourceModel(&assetsModel);
  itemView->setModel(proxyModel);
  itemView->setSortingEnabled(true);
  itemView->setRootIsDecorated(false);
  itemView->setAlternatingRowColors(true);
  itemView->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::DoubleClicked);
  itemView->setSelectionMode(QAbstractItemView::ExtendedSelection);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->addWidget(toolBar);
  layout->addWidget(itemView);
  setLayout(layout);

  connect(itemView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(itemSelectionChanged()));
  connect(&assetsModel, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(itemDataChanged(const QModelIndex&, const QModelIndex&)));
  connect(&assetsModel, SIGNAL(editedItemFlipPrice(const QModelIndex&, double)), this, SLOT(editedItemFlipPrice(const QModelIndex&, double)));

  QHeaderView* headerView = itemView->header();
  headerView->resizeSection(0, 50);
  headerView->resizeSection(1, 50);
  headerView->resizeSection(2, 110);
  headerView->resizeSection(3, 85);
  headerView->resizeSection(4, 100);
  headerView->resizeSection(5, 85);
  headerView->resizeSection(6, 100);
  headerView->resizeSection(7, 85);
  headerView->resizeSection(8, 85);
  headerView->resizeSection(9, 85);
  itemView->sortByColumn(1);
  settings.beginGroup("BotItems");
  headerView->restoreState(settings.value("HeaderState").toByteArray());
  settings.endGroup();
  headerView->setStretchLastSection(false);
  headerView->setResizeMode(0, QHeaderView::Stretch);
}

BotItemsWidget::~BotItemsWidget()
{
  entityManager.unregisterListener<EConnection>(*this);
  entityManager.unregisterListener<EUserSession>(*this);
}

void BotItemsWidget::saveState(QSettings& settings)
{
  settings.beginGroup("BotItems");
  settings.setValue("HeaderState", itemView->header()->saveState());
  settings.endGroup();
}

void BotItemsWidget::newBuyItem()
{
  addSessionItemDraft(EUserSessionAsset::Type::buy);
}

void BotItemsWidget::newSellItem()
{
  addSessionItemDraft(EUserSessionAsset::Type::sell);
}

void BotItemsWidget::submitItem()
{
  QModelIndexList selection = itemView->selectionModel()->selectedRows();
  foreach(const QModelIndex& proxyIndex, selection)
  {
    QModelIndex index = proxyModel->mapToSource(proxyIndex);
    EUserSessionAsset* eAsset = (EUserSessionAsset*)index.internalPointer();
    if(eAsset->getState() == EUserSessionAsset::State::draft)
      dataService.submitSessionAssetDraft(*(EUserSessionAssetDraft*)eAsset);
  }
}

void BotItemsWidget::cancelItem()
{
  QModelIndexList selection = itemView->selectionModel()->selectedRows();
  QList<EUserSessionAsset*> itemsToRemove;
  QList<EUserSessionAsset*> itemsToCancel;
  foreach(const QModelIndex& proxyIndex, selection)
  {
    QModelIndex index = proxyModel->mapToSource(proxyIndex);
    EUserSessionAsset* eItem = (EUserSessionAsset*)index.internalPointer();
    switch(eItem->getState())
    {
    case EUserSessionAsset::State::draft:
      itemsToRemove.append(eItem);
      break;
    case EUserSessionAsset::State::waitBuy:
    case EUserSessionAsset::State::waitSell:
      itemsToCancel.append(eItem);
      break;
    default:
      break;
    }
  }
  if(!itemsToCancel.isEmpty())
  {
    if(itemsToCancel.size() == 1)
    {
      if(QMessageBox::question(this, tr("Delete Item"), tr("Do you really want to delete the selected item?"), QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;
    }
    else if(QMessageBox::question(this, tr("Delete Items"), tr("Do you really want to delete the selected items?"), QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
      return;
  }

  while(!itemsToCancel.isEmpty())
  {
    QList<EUserSessionAsset*>::Iterator last = --itemsToCancel.end();
    dataService.removeSessionAsset(*(EUserSessionAsset*)*last);
    itemsToCancel.erase(last);
  }
  while(!itemsToRemove.isEmpty())
  {
    QList<EUserSessionAsset*>::Iterator last = --itemsToRemove.end();
    dataService.removeSessionAssetDraft(*(EUserSessionAssetDraft*)*last);
    itemsToRemove.erase(last);
  }

}

void BotItemsWidget::addSessionItemDraft(EUserSessionAsset::Type type)
{
  EConnection* eDataService = entityManager.getEntity<EConnection>(0);
  EUserSession* eSession = entityManager.getEntity<EUserSession>(eDataService->getSelectedSessionId());
  if(!eSession)
    return;
  EUserBroker* eUserBroker = entityManager.getEntity<EUserBroker>(eSession->getBrokerId());
  if(!eUserBroker)
    return;
  EBrokerType* eBrokerType = entityManager.getEntity<EBrokerType>(eUserBroker->getBrokerTypeId());
  if(!eBrokerType)
    return;
  const QString& brokerName = eBrokerType->getName();
  Entity::Manager* channelEntityManager = dataService.getSubscription(brokerName);
  double price = 0;
  if(channelEntityManager)
  {
    EMarketTickerData* eDataTickerData = channelEntityManager->getEntity<EMarketTickerData>(0);
    if(eDataTickerData)
      price = type == EUserSessionAsset::Type::buy ? (eDataTickerData->getBid() + 0.01) : (eDataTickerData->getAsk() - 0.01);
  }

  EUserSessionAssetDraft& eAssetDraft = dataService.createSessionAssetDraft(type, price);
  QModelIndex amountProxyIndex = assetsModel.getDraftAmountIndex(eAssetDraft);
  QModelIndex amountIndex = proxyModel->mapFromSource(amountProxyIndex);
  itemView->setCurrentIndex(amountIndex);
  itemView->edit(amountIndex);
}

void BotItemsWidget::itemSelectionChanged()
{
  QModelIndexList modelSelection = itemView->selectionModel()->selectedRows();
  selection.clear();
  for(QModelIndexList::Iterator i = modelSelection.begin(), end = modelSelection.end(); i != end; ++i)
  {
    QModelIndex modelIndex = proxyModel->mapToSource(*i);
    EUserSessionAsset* eItem = (EUserSessionAsset*)modelIndex.internalPointer();
    selection.insert(eItem);
  }
  updateToolBarButtons();
}

void BotItemsWidget::itemDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
  QModelIndex index = topLeft;
  for(int i = topLeft.row(), end = bottomRight.row();;)
  {
    EUserSessionAsset* eItem = (EUserSessionAsset*)index.internalPointer();
    if(selection.contains(eItem))
    {
      itemSelectionChanged();
      break;
    }
    if(i++ == end)
      break;
    index = index.sibling(i, 0);
  }
}

void BotItemsWidget::editedItemFlipPrice(const QModelIndex& index, double flipPrice)
{
  EUserSessionAsset* eitem = (EUserSessionAsset*)index.internalPointer();
  dataService.updateSessionAsset(*eitem, flipPrice);
}

void BotItemsWidget::updateToolBarButtons()
{
  EConnection* eDataService = entityManager.getEntity<EConnection>(0);
  bool connected = eDataService->getState() == EConnection::State::connected;
  bool sessionSelected = connected && eDataService->getSelectedSessionId() != 0;
  bool canCancel = !selection.isEmpty();

  bool draftSelected = false;
  for(QSet<EUserSessionAsset*>::Iterator i = selection.begin(), end = selection.end(); i != end; ++i)
  {
    EUserSessionAsset* eAsset = *i;
    if(eAsset->getState() == EUserSessionAsset::State::draft)
    {
      draftSelected = true;
      break;
    }
  }

  buyAction->setEnabled(sessionSelected);
  sellAction->setEnabled(sessionSelected);
  submitAction->setEnabled(draftSelected);
  cancelAction->setEnabled(canCancel);
}

void BotItemsWidget::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
{
  switch ((EType)newEntity.getType())
  {
  case EType::connection:
    updateToolBarButtons();
    break;
  case EType::userSession:
    {
      EConnection* eDataService = entityManager.getEntity<EConnection>(0);
      EUserSession* eSession = (EUserSession*)&newEntity;
      if(eSession->getId() == eDataService->getSelectedSessionId())
        updateToolBarButtons();
    }
    break;
  default:
    break;
  }
}
