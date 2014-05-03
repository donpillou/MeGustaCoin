
#include "stdafx.h"

OrdersWidget::OrdersWidget(QWidget* parent, QSettings& settings, Entity::Manager& entityManager, BotService& botService, DataModel& dataModel) :
  QWidget(parent), entityManager(entityManager), botService(botService), dataModel(dataModel), orderModel(entityManager)
{
  entityManager.registerListener<EBotService>(*this);
  //connect(&dataModel.orderModel, SIGNAL(changedState()), this, SLOT(updateTitle()));
  //connect(&dataModel, SIGNAL(changedMarket()), this, SLOT(updateToolBarButtons()));

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

  //connect(&orderModel, SIGNAL(editedOrder(const QModelIndex&)), this, SLOT(updateOrder(const QModelIndex&)));
  connect(orderView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), this, SLOT(updateToolBarButtons()));

  QHeaderView* headerView = orderView->header();
  headerView->resizeSection(0, 50);
  headerView->resizeSection(1, 60);
  headerView->resizeSection(2, 110);
  headerView->resizeSection(3, 85);
  headerView->resizeSection(4, 100);
  headerView->resizeSection(5, 85);
  headerView->resizeSection(5, 85);
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
  const PublicDataModel& publicDataModel = dataModel.getDataChannel(marketName);
  double price = 0;
  double bid, ask;
  if(publicDataModel.getTicker(bid, ask))
    price = type == EBotMarketOrder::Type::buy ? (bid + 0.01) : (ask - 0.01);

  EBotMarketOrderDraft& eBotMarketOrderDraft = botService.createMarketOrderDraft(type, price);
  QModelIndex amountProxyIndex = orderModel.getDraftAmountIndex(eBotMarketOrderDraft);
  QModelIndex amountIndex = proxyModel->mapFromSource(amountProxyIndex);
  orderView->setCurrentIndex(amountIndex);
  orderView->edit(amountIndex);
}

//QList<QModelIndex> OrdersWidget::getSelectedRows()
//{ // since orderView->selectionModel()->selectedRows(); does not work
//  QList<QModelIndex> result;
//  QItemSelection selection = orderView->selectionModel()->selection();
//  foreach(const QItemSelectionRange& range, selection)
//  {
//    QModelIndex parent = range.parent();
//    for(int i = range.top(), end = range.bottom() + 1; i < end; ++i)
//      result.append(range.model()->index(i, 0, parent));
//  }
//  return result;
//}

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

  //QList<QModelIndex> seletedIndices = getSelectedRows();
  //
  //foreach(const QModelIndex& proxyIndex, seletedIndices)
  //{
  //  QModelIndex index = proxyModel->mapToSource(proxyIndex);
  //  const OrderModel::Order* order = orderModel.getOrder(index);
  //  if(order->state == OrderModel::Order::State::draft)
  //  {
  //    double amount = order->newAmount != 0. ? order->newAmount : order->amount;
  //    double price = order->newPrice != 0. ? order->newPrice : order->price;
  //    /*
  //    double maxAmount = order->type == OrderModel::Order::Type::buy ? marketService.getMaxBuyAmout(price) : marketService.getMaxSellAmout();
  //    if(amount > maxAmount)
  //    {
  //      orderModel.setOrderNewAmount(order->id, maxAmount);
  //      amount = maxAmount;
  //    }
  //    */
  //
  //    marketService.createOrder(order->id, order->type == OrderModel::Order::Type::buy ? amount : -amount, price);
  //  }
  //}
  //updateToolBarButtons();
}

void OrdersWidget::cancelOrder()
{
  QModelIndexList selection = orderView->selectionModel()->selectedRows();
  QList<EBotMarketOrder*> ordersToRemove;
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
      botService.cancelMarketOrder(*eBotMarketOrder);
      break;
    default:
      break;
    }
  }
  while(!ordersToRemove.isEmpty())
  {
    QList<EBotMarketOrder*>::Iterator last = --ordersToRemove.end();
    botService.removeMarketOrderDraft(*(EBotMarketOrderDraft*)*last);
    ordersToRemove.erase(last);
  }

  //QList<QModelIndex> seletedIndices = getSelectedRows();
  //
  //QMap<int, QModelIndex> rowsToRemove;
  //foreach(const QModelIndex& proxyIndex, seletedIndices)
  //{
  //  QModelIndex index = proxyModel->mapToSource(proxyIndex);
  //  const OrderModel::Order* order = orderModel.getOrder(index);
  //  switch(order->state)
  //  {
  //  case OrderModel::Order::State::draft:
  //  case OrderModel::Order::State::canceled:
  //  case OrderModel::Order::State::closed:
  //    rowsToRemove.insert(index.row(), index);
  //    break;
  //  case OrderModel::Order::State::open:
  //    {
  //      marketService.cancelOrder(order->id);
  //      break;
  //    }
  //  default:
  //    break;
  //  }
  //}
  //while(!rowsToRemove.isEmpty())
  //{
  //  QMap<int, QModelIndex>::iterator last = --rowsToRemove.end();
  //  orderModel.removeOrder(last.value());
  //  rowsToRemove.erase(last);
  //}
  //updateToolBarButtons();
}

void OrdersWidget::updateOrder(const QModelIndex& index)
{
  //const OrderModel::Order* order = orderModel.getOrder(index);
  //if(order->state != OrderModel::Order::State::open || (order->newAmount == 0. && order->newPrice == 0.))
  //  return;
  //double amount = order->newAmount != 0. ? order->newAmount : order->amount;
  //double price = order->newPrice != 0. ? order->newPrice : order->price;
  //
  //marketService.updateOrder(order->id, order->type == OrderModel::Order::Type::buy ? amount : -amount, price);
  //updateToolBarButtons();
}

void OrdersWidget::updateToolBarButtons()
{
  QModelIndexList selection = orderView->selectionModel()->selectedRows();
  EBotService* eBotService = entityManager.getEntity<EBotService>(0);
  bool connected = botService.isConnected();
  bool marketSelected = connected && eBotService->getSelectedMarketId() != 0;
  bool canCancel = selection.size() > 0;

  bool draftSelected = false;
  foreach(const QModelIndex& proxyIndex, selection)
  {
    QModelIndex index = proxyModel->mapToSource(proxyIndex);
    EBotMarketOrder* eBotMarketOrder = (EBotMarketOrder*)index.internalPointer();
    if(eBotMarketOrder->getState() == EBotMarketOrder::State::draft)
    {
      draftSelected = true;
      break;
    }
  }

  //QList<QModelIndex> selectedRows = getSelectedRows();
  //
  //bool hasMarket = marketService.isReady();
  //bool canCancel = getSelectedRows().size() > 0;
  //bool canSubmit = false;
  //
  //if(hasMarket)
  //  foreach(const QModelIndex& proxyIndex, selectedRows)
  //  {
  //    QModelIndex index = proxyModel->mapToSource(proxyIndex);
  //    const OrderModel::Order* order = orderModel.getOrder(index);
  //    if(order->state == OrderModel::Order::State::draft)
  //    {
  //      canSubmit = true;
  //      break;
  //    }
  //  }
  //
  refreshAction->setEnabled(marketSelected);
  buyAction->setEnabled(marketSelected);
  sellAction->setEnabled(marketSelected);
  submitAction->setEnabled(marketSelected && draftSelected);
  cancelAction->setEnabled(canCancel);
}

void OrdersWidget::refresh()
{
  botService.refreshMarketOrders();
  //marketService.loadOrders();
  //marketService.loadBalance();
}

void OrdersWidget::updateTitle(EBotService& eBotService)
{
  QString stateStr = eBotService.getStateName();
  QString title;
  if(stateStr.isEmpty())
    title = tr("Orders");
  else
    title = tr("Orders (%2)").arg(stateStr);
  QDockWidget* dockWidget = qobject_cast<QDockWidget*>(parent());
  dockWidget->setWindowTitle(title);
  dockWidget->toggleViewAction()->setText(tr("Orders"));
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
