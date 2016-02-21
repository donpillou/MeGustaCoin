
#include "stdafx.h"

UserSessionAssetsWidget::UserSessionAssetsWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager, DataService& dataService) :
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

  importAction = toolBar->addAction(QIcon(":/Icons/table_import.png"), tr("&Import"));
  importAction->setEnabled(false);
  connect(importAction, SIGNAL(triggered()), this, SLOT(importAssets()));

  exportAction = toolBar->addAction(QIcon(":/Icons/table_export.png"), tr("&Export"));
  exportAction->setEnabled(false);
  connect(exportAction, SIGNAL(triggered()), this, SLOT(exportAssets()));

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

UserSessionAssetsWidget::~UserSessionAssetsWidget()
{
  entityManager.unregisterListener<EConnection>(*this);
  entityManager.unregisterListener<EUserSession>(*this);
}

void UserSessionAssetsWidget::saveState(QSettings& settings)
{
  settings.beginGroup("BotItems");
  settings.setValue("HeaderState", itemView->header()->saveState());
  settings.endGroup();
}

void UserSessionAssetsWidget::newBuyItem()
{
  addSessionItemDraft(EUserSessionAsset::Type::buy);
}

void UserSessionAssetsWidget::newSellItem()
{
  addSessionItemDraft(EUserSessionAsset::Type::sell);
}

void UserSessionAssetsWidget::submitItem()
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

void UserSessionAssetsWidget::cancelItem()
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

void UserSessionAssetsWidget::importAssets()
{
  QString fileName = QFileDialog::getOpenFileName(parentWidget(), tr("Import Assets"), QString(), tr("JSON File (*.json)"));
  if(fileName.isEmpty())
    return;

  QFile file(fileName);
  if(!file.open(QIODevice::ReadOnly))
    return (void)QMessageBox::critical(parentWidget(), tr("Import Failed"), tr("Could not open file \"%1\": %2").arg(fileName, file.errorString()));

  const QVariantList list = Json::parse(file.readAll()).toList();
  foreach(const QVariant& assetVar, list)
  {
    const QVariantMap assetData = assetVar.toMap();

    EUserSessionAsset::Type type = (EUserSessionAsset::Type)assetData["type"].toInt();
    switch(type)
    {
    case EUserSessionAsset::Type::buy:
    case EUserSessionAsset::Type::sell:
      break;
    default:
      dataService.addLogMessage(ELogMessage::Type::warning, tr("Skipped session with invalid type"));
      continue;
    }
    EUserSessionAsset::State state = (EUserSessionAsset::State)assetData["state"].toInt();
    switch(state)
    {
    case EUserSessionAsset::State::waitBuy:
    case EUserSessionAsset::State::waitSell:
      break;
    default:
      dataService.addLogMessage(ELogMessage::Type::warning, tr("Skipped session with invalid state"));
      continue;
    }
    double price = assetData["price"].toDouble();
    double investComm = assetData["investComm"].toDouble();
    double investBase = assetData["investBase"].toDouble();
    double balanceComm = assetData["balanceComm"].toDouble();
    double balanceBase = assetData["balanceBase"].toDouble();
    double profitablePrice = assetData["profitablePrice"].toDouble();
    double flipPrice = assetData["flipPrice"].toDouble();

    dataService.createSessionAsset(type, state, price, investComm, investBase, balanceComm, balanceBase, profitablePrice, flipPrice);
  }
}

void UserSessionAssetsWidget::exportAssets()
{
  QString fileName = QFileDialog::getSaveFileName(parentWidget(), tr("Export Selected Assets"), QString(), tr("JSON File (*.json)"));
  if(fileName.isEmpty())
    return;

  QVariantList data;
  QModelIndexList selection = itemView->selectionModel()->selectedRows();
  foreach(const QModelIndex& proxyIndex, selection)
  {
    QModelIndex index = proxyModel->mapToSource(proxyIndex);
    EUserSessionAsset* eAsset = (EUserSessionAsset*)index.internalPointer();
    if(eAsset->getState() != EUserSessionAsset::State::waitBuy && eAsset->getState() != EUserSessionAsset::State::waitSell)
      continue;

    QVariantMap assetData;
    assetData["id"] = eAsset->getId();
    assetData["type"] = (int)eAsset->getType();
    assetData["state"] = (int)eAsset->getState();
    assetData["date"] = eAsset->getDate().toMSecsSinceEpoch();
    assetData["price"] = eAsset->getPrice();
    assetData["investComm"] = eAsset->getInvestComm();
    assetData["investBase"] = eAsset->getInvestBase();
    assetData["balanceComm"] = eAsset->getBalanceComm();
    assetData["balanceBase"] = eAsset->getBalanceBase();
    assetData["profitablePrice"] = eAsset->getProfitablePrice();
    assetData["flipPrice"] = eAsset->getFlipPrice();
    assetData["orderId"] = eAsset->getOrderId();
    data.append(assetData);
  }

  QFile file(fileName);
  if(!file.open(QIODevice::WriteOnly))
    return (void)QMessageBox::critical(parentWidget(), tr("Export Failed"), tr("Could not open file \"%1\": %2").arg(fileName, file.errorString()));
  file.write(Json::generate(data));
}

void UserSessionAssetsWidget::addSessionItemDraft(EUserSessionAsset::Type type)
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

void UserSessionAssetsWidget::itemSelectionChanged()
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

void UserSessionAssetsWidget::itemDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
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

void UserSessionAssetsWidget::editedItemFlipPrice(const QModelIndex& index, double flipPrice)
{
  EUserSessionAsset* eitem = (EUserSessionAsset*)index.internalPointer();
  dataService.updateSessionAsset(*eitem, flipPrice);
}

void UserSessionAssetsWidget::updateToolBarButtons()
{
  EConnection* eDataService = entityManager.getEntity<EConnection>(0);
  bool connected = eDataService->getState() == EConnection::State::connected;
  bool sessionSelected = connected && eDataService->getSelectedSessionId() != 0;
  bool canCancel = !selection.isEmpty();

  bool draftSelected = false;
  bool canExport = false;
  for(QSet<EUserSessionAsset*>::Iterator i = selection.begin(), end = selection.end(); i != end; ++i)
  {
    EUserSessionAsset* eAsset = *i;
    switch(eAsset->getState())
    {
    case EUserSessionAsset::State::draft:
      draftSelected = true;
      break;
    case EUserSessionAsset::State::waitBuy:
    case EUserSessionAsset::State::waitSell:
      canExport = true;
      break;
    default:
      break;
    }
  }

  buyAction->setEnabled(sessionSelected);
  sellAction->setEnabled(sessionSelected);
  submitAction->setEnabled(draftSelected);
  cancelAction->setEnabled(canCancel);
  exportAction->setEnabled(canExport);
  importAction->setEnabled(sessionSelected);
}

void UserSessionAssetsWidget::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
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
