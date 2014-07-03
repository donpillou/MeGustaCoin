
#include "stdafx.h"

OrdersWidget::OrdersWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager, BotService& botService, DataService& dataService) :
  QWidget(&tabFramework), tabFramework(tabFramework), entityManager(entityManager), botService(botService), dataService(dataService), orderModel(entityManager)
{
  entityManager.registerListener<EBotService>(*this);

  setWindowTitle(tr("Orders"));

  QToolBar* toolBar = new QToolBar(this);
  toolBar->setStyleSheet("QToolBar { border: 0px }");
  toolBar->setIconSize(QSize(16, 16));
  toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  refreshAction = toolBar->addAction(QIcon(":/Icons/arrow_refresh.png"), tr("&Refresh"));
  refreshAction->setEnabled(false);
  refreshAction->setShortcut(QKeySequence(QKeySequence::Refresh));
  connect(refreshAction, SIGNAL(triggered()), this, SLOT(refresh()));
  
  buyAction = toolBar->addAction(QIcon(":/Icons/bitcoin_add.png"), tr("&Buy"));
  buyAction->setEnabled(false);
  buyAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_B));
  connect(buyAction, SIGNAL(triggered()), this, SLOT(newBuyOrder()));
  sellAction = toolBar->addAction(QIcon(":/Icons/money_add.png"), tr("&Sell"));
  sellAction->setEnabled(false);
  sellAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_S));
  connect(sellAction, SIGNAL(triggered()), this, SLOT(newSellOrder()));

  submitAction = toolBar->addAction(QIcon(":/Icons/bullet_go.png"), tr("S&ubmit"));
  submitAction->setEnabled(false);
  submitAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_U));
  connect(submitAction, SIGNAL(triggered()), this, SLOT(submitOrder()));

  cancelAction = toolBar->addAction(QIcon(":/Icons/cancel2.png"), tr("&Cancel"));
  cancelAction->setEnabled(false);
  cancelAction->setShortcut(QKeySequence(Qt::Key_Delete));
  connect(cancelAction, SIGNAL(triggered()), this, SLOT(cancelOrder()));

  orderView = new QTreeView(this);
  orderView->setUniformRowHeights(true);
  proxyModel = new MarketOrderSortProxyModel(this);
  proxyModel->setDynamicSortFilter(true);
  proxyModel->setSourceModel(&orderModel);
  orderView->setModel(proxyModel);
  orderView->setSortingEnabled(true);
  orderView->setRootIsDecorated(false);
  orderView->setAlternatingRowColors(true);
  orderView->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::DoubleClicked);
  orderView->setSelectionMode(QAbstractItemView::ExtendedSelection);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->addWidget(toolBar);
  layout->addWidget(orderView);
  setLayout(layout);

  connect(&orderModel, SIGNAL(editedOrderPrice(const QModelIndex&, double)), this, SLOT(editedOrderPrice(const QModelIndex&, double)));
  connect(&orderModel, SIGNAL(editedOrderAmount(const QModelIndex&, double)), this, SLOT(editedOrderAmount(const QModelIndex&, double)));
  connect(orderView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(orderSelectionChanged()));
  connect(&orderModel, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(orderDataChanged(const QModelIndex&, const QModelIndex&)));

  QHeaderView* headerView = orderView->header();
  headerView->resizeSection(0, 50);
  headerView->resizeSection(1, 60);
  headerView->resizeSection(2, 110);
  headerView->resizeSection(3, 85);
  headerView->resizeSection(4, 100);
  headerView->resizeSection(5, 85);
  headerView->resizeSection(6, 85);
  headerView->setStretchLastSection(false);
  headerView->setResizeMode(0, QHeaderView::Stretch);
  orderView->sortByColumn(2);
  settings.beginGroup("Orders");
  headerView->restoreState(settings.value("HeaderState").toByteArray());
  settings.endGroup();
}

OrdersWidget::~OrdersWidget()
{
  entityManager.unregisterListener<EBotService>(*this);
}

void OrdersWidget::saveState(QSettings& settings)
{
  settings.beginGroup("Orders");
  settings.setValue("HeaderState", orderView->header()->saveState());
  settings.endGroup();
}

void OrdersWidget::newBuyOrder()
{
  addOrderDraft(EBotMarketOrder::Type::buy);
}

void OrdersWidget::newSellOrder()
{
  addOrderDraft(EBotMarketOrder::Type::sell);
}

void OrdersWidget::addOrderDraft(EBotMarketOrder::Type type)
{
  EBotService* eBotService = entityManager.getEntity<EBotService>(0);
  EBotMarket* eBotMarket = entityManager.getEntity<EBotMarket>(eBotService->getSelectedMarketId());
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
      price = type == EBotMarketOrder::Type::buy ? (eDataTickerData->getBid() + 0.01) : (eDataTickerData->getAsk() - 0.01);
  }

  EBotMarketOrderDraft& eBotMarketOrderDraft = botService.createMarketOrderDraft(type, price);
  QModelIndex amountProxyIndex = orderModel.getDraftAmountIndex(eBotMarketOrderDraft);
  QModelIndex amountIndex = proxyModel->mapFromSource(amountProxyIndex);
  orderView->setCurrentIndex(amountIndex);
  orderView->edit(amountIndex);
}

void OrdersWidget::submitOrder()
{
  QModelIndexList selection = orderView->selectionModel()->selectedRows();
  foreach(const QModelIndex& proxyIndex, selection)
  {
    QModelIndex index = proxyModel->mapToSource(proxyIndex);
    EBotMarketOrder* eBotMarketOrder = (EBotMarketOrder*)index.internalPointer();
    if(eBotMarketOrder->getState() == EBotMarketOrder::State::draft)
      botService.submitMarketOrderDraft(*(EBotMarketOrderDraft*)eBotMarketOrder);
  }
}

void OrdersWidget::cancelOrder()
{
  QModelIndexList selection = orderView->selectionModel()->selectedRows();
  QList<EBotMarketOrder*> ordersToRemove;
  QList<EBotMarketOrder*> ordersToCancel;
  foreach(const QModelIndex& proxyIndex, selection)
  {
    QModelIndex index = proxyModel->mapToSource(proxyIndex);
    EBotMarketOrder* eBotMarketOrder = (EBotMarketOrder*)index.internalPointer();
    switch(eBotMarketOrder->getState())
    {
    case EBotMarketOrder::State::draft:
    case EBotMarketOrder::State::canceled:
    case EBotMarketOrder::State::closed:
      ordersToRemove.append(eBotMarketOrder);
      break;
    case EBotMarketOrder::State::open:
      ordersToCancel.append(eBotMarketOrder);
      break;
    default:
      break;
    }
  }
  while(!ordersToCancel.isEmpty())
  {
    QList<EBotMarketOrder*>::Iterator last = --ordersToCancel.end();
    botService.cancelMarketOrder(*(EBotMarketOrder*)*last);
    ordersToCancel.erase(last);
  }
  while(!ordersToRemove.isEmpty())
  {
    QList<EBotMarketOrder*>::Iterator last = --ordersToRemove.end();
    botService.removeMarketOrderDraft(*(EBotMarketOrderDraft*)*last);
    ordersToRemove.erase(last);
  }
}

void OrdersWidget::editedOrderPrice(const QModelIndex& index, double price)
{
  EBotMarketOrder* eBotMarketOrder = (EBotMarketOrder*)index.internalPointer();
  botService.updateMarketOrder(*eBotMarketOrder, price, eBotMarketOrder->getAmount());
}

void OrdersWidget::editedOrderAmount(const QModelIndex& index, double amount)
{
  EBotMarketOrder* eBotMarketOrder = (EBotMarketOrder*)index.internalPointer();
  botService.updateMarketOrder(*eBotMarketOrder, eBotMarketOrder->getPrice(), amount);
}

void OrdersWidget::orderSelectionChanged()
{
  QModelIndexList modelSelection = orderView->selectionModel()->selectedRows();
  selection.clear();
  if(!modelSelection.isEmpty())
  {
    QModelIndex modelIndex = proxyModel->mapToSource(modelSelection.front());
    EBotMarketOrder* eOrder = (EBotMarketOrder*)modelIndex.internalPointer();
    selection.insert(eOrder);
  }
  updateToolBarButtons();
}

void OrdersWidget::orderDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
  QModelIndex index = topLeft;
  for(int i = topLeft.row(), end = bottomRight.row();;)
  {
    EBotMarketOrder* eOrder = (EBotMarketOrder*)index.internalPointer();
    if(selection.contains(eOrder))
    {
      orderSelectionChanged();
      break;
    }
    if(i++ == end)
      break;
    index = index.sibling(i, 0);
  }
}

void OrdersWidget::updateToolBarButtons()
{
  EBotService* eBotService = entityManager.getEntity<EBotService>(0);
  bool connected = eBotService->getState() == EBotService::State::connected;
  bool marketSelected = connected && eBotService->getSelectedMarketId() != 0;
  bool canCancel = !selection.isEmpty();

  bool draftSelected = false;
  for(QSet<EBotMarketOrder*>::Iterator i = selection.begin(), end = selection.end(); i != end; ++i)
  {
    EBotMarketOrder* eBotMarketOrder = *i;
    if(eBotMarketOrder->getState() == EBotMarketOrder::State::draft)
    {
      draftSelected = true;
      break;
    }
  }

  refreshAction->setEnabled(marketSelected);
  buyAction->setEnabled(marketSelected);
  sellAction->setEnabled(marketSelected);
  submitAction->setEnabled(marketSelected && draftSelected);
  cancelAction->setEnabled(canCancel);
}

void OrdersWidget::refresh()
{
  botService.refreshMarketOrders();
  botService.refreshMarketBalance();
}

void OrdersWidget::updateTitle(EBotService& eBotService)
{
  QString stateStr = eBotService.getStateName();
  QString title;
  if(stateStr.isEmpty())
    title = tr("Orders");
  else
    title = tr("Orders (%2)").arg(stateStr);

  setWindowTitle(title);
  tabFramework.toggleViewAction(this)->setText(tr("Orders"));
}

void OrdersWidget::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
{
  switch ((EType)newEntity.getType())
  {
  case EType::botService:
    updateTitle(*dynamic_cast<EBotService*>(&newEntity));
    updateToolBarButtons();
    break;
  default:
    break;
  }
}
