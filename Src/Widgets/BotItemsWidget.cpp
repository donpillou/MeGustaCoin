
#include "stdafx.h"

BotItemsWidget::BotItemsWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager, BotService& botService, DataService& dataService) :
  QWidget(&tabFramework), tabFramework(tabFramework), entityManager(entityManager), botService(botService), dataService(dataService), itemModel(entityManager)
{
  entityManager.registerListener<EBotService>(*this);
  entityManager.registerListener<EBotSession>(*this);

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

  exportAction = toolBar->addAction(QIcon(":/Icons/table_export.png"), tr("&Export"));
  exportAction->setEnabled(false);
  connect(exportAction, SIGNAL(triggered()), this, SLOT(exportAssets()));

  itemView = new QTreeView(this);
  itemView->setUniformRowHeights(true);
  proxyModel = new SessionItemSortProxyModel(this);
  proxyModel->setDynamicSortFilter(true);
  proxyModel->setSourceModel(&itemModel);
  itemView->setModel(proxyModel);
  itemView->setSortingEnabled(true);
  itemView->setRootIsDecorated(false);
  itemView->setAlternatingRowColors(true);
  itemView->setItemDelegateForColumn((int)SessionItemModel::Column::balanceComm, new DecimalDelegate(8, this));
  itemView->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::DoubleClicked);
  itemView->setSelectionMode(QAbstractItemView::ExtendedSelection);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->addWidget(toolBar);
  layout->addWidget(itemView);
  setLayout(layout);

  connect(itemView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(itemSelectionChanged()));
  connect(&itemModel, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(itemDataChanged(const QModelIndex&, const QModelIndex&)));
  connect(&itemModel, SIGNAL(editedItemFlipPrice(const QModelIndex&, double)), this, SLOT(editedItemFlipPrice(const QModelIndex&, double)));

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
  entityManager.unregisterListener<EBotService>(*this);
  entityManager.unregisterListener<EBotSession>(*this);
}

void BotItemsWidget::saveState(QSettings& settings)
{
  settings.beginGroup("BotItems");
  settings.setValue("HeaderState", itemView->header()->saveState());
  settings.endGroup();
}

void BotItemsWidget::newBuyItem()
{
  addSessionItemDraft(EBotSessionItem::Type::buy);
}

void BotItemsWidget::newSellItem()
{
  addSessionItemDraft(EBotSessionItem::Type::sell);
}

void BotItemsWidget::submitItem()
{
  QModelIndexList selection = itemView->selectionModel()->selectedRows();
  foreach(const QModelIndex& proxyIndex, selection)
  {
    QModelIndex index = proxyModel->mapToSource(proxyIndex);
    EBotSessionItem* eBotSessionItem = (EBotSessionItem*)index.internalPointer();
    if(eBotSessionItem->getState() == EBotSessionItem::State::draft)
      botService.submitSessionItemDraft(*(EBotSessionItemDraft*)eBotSessionItem);
  }
}

void BotItemsWidget::cancelItem()
{
  QModelIndexList selection = itemView->selectionModel()->selectedRows();
  QList<EBotSessionItem*> itemsToRemove;
  QList<EBotSessionItem*> itemsToCancel;
  foreach(const QModelIndex& proxyIndex, selection)
  {
    QModelIndex index = proxyModel->mapToSource(proxyIndex);
    EBotSessionItem* eItem = (EBotSessionItem*)index.internalPointer();
    switch(eItem->getState())
    {
    case EBotSessionItem::State::draft:
      itemsToRemove.append(eItem);
      break;
    case EBotSessionItem::State::waitBuy:
    case EBotSessionItem::State::waitSell:
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
    QList<EBotSessionItem*>::Iterator last = --itemsToCancel.end();
    botService.cancelSessionItem(*(EBotSessionItem*)*last);
    itemsToCancel.erase(last);
  }
  while(!itemsToRemove.isEmpty())
  {
    QList<EBotSessionItem*>::Iterator last = --itemsToRemove.end();
    botService.removeSessionItemDraft(*(EBotSessionItemDraft*)*last);
    itemsToRemove.erase(last);
  }
}

void BotItemsWidget::exportAssets()
{
  QString fileName = QFileDialog::getSaveFileName(parentWidget(), tr("Export Selected Assets"), QString(), tr("JSON File (*.json)"));
  if(fileName.isEmpty())
    return;

  QVariantList data;
  QModelIndexList selection = itemView->selectionModel()->selectedRows();
  foreach(const QModelIndex& proxyIndex, selection)
  {
    QModelIndex index = proxyModel->mapToSource(proxyIndex);
    EBotSessionItem* eAsset = (EBotSessionItem*)index.internalPointer();
    if(eAsset->getState() != EBotSessionItem::State::waitBuy && eAsset->getState() != EBotSessionItem::State::waitSell)
      continue;

    QVariantMap assetData;
    assetData["id"] = eAsset->getId();
    assetData["type"] = (int)eAsset->getType();
    assetData["state"] = (int)eAsset->getState() + 1;
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

void BotItemsWidget::addSessionItemDraft(EBotSessionItem::Type type)
{
  EBotService* eBotService = entityManager.getEntity<EBotService>(0);
  EBotSession* eBotSession = entityManager.getEntity<EBotSession>(eBotService->getSelectedSessionId());
  if(!eBotSession)
    return;
  EBotMarket* eBotMarket = entityManager.getEntity<EBotMarket>(eBotSession->getMarketId());
  if(!eBotMarket)
    return;
  EBotMarketAdapter* eBotMarketAdapater = entityManager.getEntity<EBotMarketAdapter>(eBotMarket->getMarketAdapterId());
  if(!eBotMarketAdapater)
    return;
  const QString& marketName = eBotMarketAdapater->getName();
  Entity::Manager* channelEntityManager = dataService.getSubscription(marketName);
  double price = 0;
  if(channelEntityManager)
  {
    EDataTickerData* eDataTickerData = channelEntityManager->getEntity<EDataTickerData>(0);
    if(eDataTickerData)
      price = type == EBotSessionItem::Type::buy ? (eDataTickerData->getBid() + 0.01) : (eDataTickerData->getAsk() - 0.01);
  }

  EBotSessionItemDraft& eBotSessionItemDraft = botService.createSessionItemDraft(type, price);
  QModelIndex amountProxyIndex = itemModel.getDraftAmountIndex(eBotSessionItemDraft);
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
    EBotSessionItem* eItem = (EBotSessionItem*)modelIndex.internalPointer();
    selection.insert(eItem);
  }
  updateToolBarButtons();
}

void BotItemsWidget::itemDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
  QModelIndex index = topLeft;
  for(int i = topLeft.row(), end = bottomRight.row();;)
  {
    EBotSessionItem* eItem = (EBotSessionItem*)index.internalPointer();
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
  EBotSessionItem* eitem = (EBotSessionItem*)index.internalPointer();
  botService.updateSessionItem(*eitem, flipPrice);
}

void BotItemsWidget::updateToolBarButtons()
{
  EBotService* eBotService = entityManager.getEntity<EBotService>(0);
  bool connected = eBotService->getState() == EBotService::State::connected;
  bool sessionSelected = connected && eBotService->getSelectedSessionId() != 0;
  bool canCancel = !selection.isEmpty();

  bool draftSelected = false;
  bool canExport = false;
  for(QSet<EBotSessionItem*>::Iterator i = selection.begin(), end = selection.end(); i != end; ++i)
  {
    EBotSessionItem* eBotMarketOrder = *i;
    switch(eBotMarketOrder->getState())
    {
    case EBotSessionItem::State::draft:
      draftSelected = true;
      break;
    case EBotSessionItem::State::waitBuy:
    case EBotSessionItem::State::waitSell:
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
}

void BotItemsWidget::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
{
  switch ((EType)newEntity.getType())
  {
  case EType::botService:
    updateToolBarButtons();
    break;
  case EType::botSession:
    {
      EBotService* eBotService = entityManager.getEntity<EBotService>(0);
      EBotSession* eSession = (EBotSession*)&newEntity;
      if(eSession->getId() == eBotService->getSelectedSessionId())
        updateToolBarButtons();
    }
    break;
  default:
    break;
  }
}
