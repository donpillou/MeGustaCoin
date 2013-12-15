
#include "stdafx.h"

OrdersWidget::OrdersWidget(QWidget* parent, QSettings& settings, DataModel& dataModel) : QWidget(parent), dataModel(dataModel), orderModel(dataModel.orderModel)
{
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
  proxyModel = new QSortFilterProxyModel(this);
  proxyModel->setDynamicSortFilter(true);
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

  connect(&orderModel, SIGNAL(orderEdited(const QModelIndex&)), this, SLOT(updateOrder(const QModelIndex&)));
  connect(orderView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), this, SLOT(updateToolBarButtons()));

  proxyModel->setSourceModel(&orderModel);
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
  headerView->restoreState(settings.value("OrderHeaderState").toByteArray());
}

void OrdersWidget::saveState(QSettings& settings)
{
  settings.setValue("OrderHeaderState", orderView->header()->saveState());
}

void OrdersWidget::setMarket(Market* market)
{
  this->market = market;
  updateToolBarButtons();
}

void OrdersWidget::newBuyOrder()
{
  addOrder(OrderModel::Order::Type::buy);
}

void OrdersWidget::newSellOrder()
{
  addOrder(OrderModel::Order::Type::sell);
}

void OrdersWidget::addOrder(OrderModel::Order::Type type)
{
  if(!market)
    return;

  double price = 0;
  const Market::TickerData* tickerData = market->getTickerData();
  if(tickerData)
    price = type == OrderModel::Order::Type::buy ? (tickerData->highestBuyOrder + 0.01) : (tickerData->lowestSellOrder - 0.01);

  int row = orderModel.addOrder(type, price);

  QModelIndex index = proxyModel->mapFromSource(orderModel.index(row, (int)OrderModel::Column::amount));
  orderView->setCurrentIndex(index);
  orderView->edit(index);
}

QList<QModelIndex> OrdersWidget::getSelectedRows()
{ // since orderView->selectionModel()->selectedRows(); does not work
  QList<QModelIndex> result;
  QItemSelection selection = orderView->selectionModel()->selection();
  foreach(const QItemSelectionRange& range, selection)
  {
    QModelIndex parent = range.parent();
    for(int i = range.top(), end = range.bottom() + 1; i < end; ++i)
      result.append(range.model()->index(i, 0, parent));
  }
  return result;
}

void OrdersWidget::submitOrder()
{
  QList<QModelIndex> seletedIndices = getSelectedRows();

  foreach(const QModelIndex& proxyIndex, seletedIndices)
  {
    QModelIndex index = proxyModel->mapToSource(proxyIndex);
    const OrderModel::Order* order = orderModel.getOrder(index);
    if(order->state == OrderModel::Order::State::draft)
    {
      double amount = order->newAmount != 0. ? order->newAmount : order->amount;
      double price = order->newPrice != 0. ? order->newPrice : order->price;
      double maxAmount = order->type == OrderModel::Order::Type::buy ? market->getMaxBuyAmout(price) : market->getMaxSellAmout();
      if(amount > maxAmount)
      {
        orderModel.setOrderNewAmount(order->id, maxAmount);
        amount = maxAmount;
      }

      market->createOrder(order->id, order->type == OrderModel::Order::Type::buy ? amount : -amount, price);
    }
  }
}

void OrdersWidget::cancelOrder()
{
  QList<QModelIndex> seletedIndices = getSelectedRows();

  QMap<int, QModelIndex> rowsToRemove;
  foreach(const QModelIndex& proxyIndex, seletedIndices)
  {
    QModelIndex index = proxyModel->mapToSource(proxyIndex);
    const OrderModel::Order* order = orderModel.getOrder(index);
    switch(order->state)
    {
    case OrderModel::Order::State::draft:
    case OrderModel::Order::State::canceled:
    case OrderModel::Order::State::closed:
      rowsToRemove.insert(index.row(), index);
      break;
    case OrderModel::Order::State::open:
      {
        market->cancelOrder(order->id, order->type == OrderModel::Order::Type::buy ? order->amount : -order->amount, order->price);
        break;
      }
    }
  }
  while(!rowsToRemove.isEmpty())
  {
    QMap<int, QModelIndex>::iterator last = --rowsToRemove.end();
    orderModel.removeOrder(last.value());
    rowsToRemove.erase(last);
  }
}

void OrdersWidget::updateOrder(const QModelIndex& index)
{
  const OrderModel::Order* order = orderModel.getOrder(index);
  if(order->state != OrderModel::Order::State::open || (order->newAmount == 0. && order->newPrice == 0.))
    return;
  double amount = order->newAmount != 0. ? order->newAmount : order->amount;
  double price = order->newPrice != 0. ? order->newPrice : order->price;

  double maxAmount = order->type == OrderModel::Order::Type::buy ? market->getMaxBuyAmout(price, order->amount, order->price) : market->getMaxSellAmout() + order->amount;
  if(amount > maxAmount)
  {
    orderModel.setOrderNewAmount(order->id, maxAmount);
    amount = maxAmount;
  }

  if(order->type == OrderModel::Order::Type::buy)
    market->updateOrder(order->id, amount, price, order->amount, order->price);
  else
    market->updateOrder(order->id, -amount, price, -order->amount, order->price);
}

void OrdersWidget::updateToolBarButtons()
{
  QList<QModelIndex> selectedRows = getSelectedRows();

  bool hasMarket = market != 0;
  bool canCancel = getSelectedRows().size() > 0;
  bool canSubmit = false;

  if(hasMarket)
    foreach(const QModelIndex& proxyIndex, selectedRows)
    {
      QModelIndex index = proxyModel->mapToSource(proxyIndex);
      const OrderModel::Order* order = orderModel.getOrder(index);
      if(order->state == OrderModel::Order::State::draft)
      {
        canSubmit = true;
        break;
      }
    }

  refreshAction->setEnabled(hasMarket);
  buyAction->setEnabled(hasMarket);
  sellAction->setEnabled(hasMarket);
  submitAction->setEnabled(canSubmit);
  cancelAction->setEnabled(canCancel);
}

void OrdersWidget::refresh()
{
  if(!market)
    return;
  dataModel.logModel.addMessage(LogModel::Type::information, "Refreshing orders...");
  market->loadBalance(); // load fee
  market->loadOrders();
  market->loadTicker();
}
