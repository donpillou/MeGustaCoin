
#include "stdafx.h"

OrdersWidget::OrdersWidget(QWidget* parent, QSettings& settings) : QWidget(parent), settings(settings)
{
  QToolBar* toolBar = new QToolBar(this);
  toolBar->setStyleSheet("QToolBar { border: 0px }");
  toolBar->setIconSize(QSize(16, 16));
  toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  refreshAction = toolBar->addAction(QIcon(":/Icons/arrow_refresh.png"), tr("&Refresh"));
  refreshAction->setEnabled(false);
  refreshAction->setShortcut(QKeySequence(QKeySequence::Refresh));
  connect(refreshAction, SIGNAL(triggered()), parent, SLOT(refresh()));
  
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

  cancelAction = toolBar->addAction(QIcon(":/Icons/cancel.png"), tr("&Cancel"));
  cancelAction->setEnabled(false);
  cancelAction->setShortcut(QKeySequence(Qt::Key_Delete));
  connect(cancelAction, SIGNAL(triggered()), this, SLOT(cancelOrder()));

  orderView = new QTreeView(this);
  orderProxyModel = new QSortFilterProxyModel(this);
  orderProxyModel->setDynamicSortFilter(true);
  orderView->setModel(orderProxyModel);
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
}

void OrdersWidget::setMarket(Market* market)
{
  this->market = market;
  if(!market)
  {
    settings.setValue("OrderHeaderState", orderView->header()->saveState());
    orderProxyModel->setSourceModel(0);
  }
  else
  {
    OrderModel& orderModel = market->getOrderModel();
    connect(&orderModel, SIGNAL(orderEdited(const QModelIndex&)), this, SLOT(updateOrder(const QModelIndex&)));
    connect(orderView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), this, SLOT(updateToolBarButtons()));

    orderProxyModel->setSourceModel(&orderModel);
    orderView->header()->resizeSection(0, 85);
    orderView->header()->resizeSection(1, 85);
    orderView->header()->resizeSection(2, 150);
    orderView->header()->resizeSection(3, 85);
    orderView->header()->resizeSection(4, 85);
    orderView->header()->resizeSection(5, 85);
    orderView->header()->restoreState(settings.value("OrderHeaderState").toByteArray());
  }
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

  OrderModel& orderModel = market->getOrderModel();
  int row = orderModel.addOrder(type, price);

  QModelIndex index = orderProxyModel->mapFromSource(orderModel.index(row, (int)OrderModel::Column::amount));
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

  OrderModel& orderModel = market->getOrderModel();
  foreach(const QModelIndex& proxyIndex, seletedIndices)
  {
    QModelIndex index = orderProxyModel->mapToSource(proxyIndex);
    const OrderModel::Order* order = orderModel.getOrder(index);
    if(order->state == OrderModel::Order::State::draft)
    {
      market->createOrder(order->id, order->type == OrderModel::Order::Type::sell, order->amount, order->price);
    }
  }
}

void OrdersWidget::cancelOrder()
{
  QList<QModelIndex> seletedIndices = getSelectedRows();

  OrderModel& orderModel = market->getOrderModel();
  QMap<int, QModelIndex> rowsToRemove;
  foreach(const QModelIndex& proxyIndex, seletedIndices)
  {
    QModelIndex index = orderProxyModel->mapToSource(proxyIndex);
    const OrderModel::Order* order = orderModel.getOrder(index);
    switch(order->state)
    {
    case OrderModel::Order::State::draft:
    case OrderModel::Order::State::canceled:
      rowsToRemove.insert(index.row(), index);
      break;
    case OrderModel::Order::State::open:
      market->cancelOrder(order->id);
      break;
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
  OrderModel& orderModel = market->getOrderModel();
  const OrderModel::Order* order = orderModel.getOrder(index);
  if(order->state != OrderModel::Order::State::open || (order->newAmount == 0. && order->newPrice == 0.))
    return;
  double amount = order->newAmount != 0. ? order->newAmount : order->amount;
  double price = order->newPrice != 0. ? order->newPrice : order->price;
  market->updateOrder(order->id, order->type == OrderModel::Order::Type::sell, amount, price);
}

void OrdersWidget::updateToolBarButtons()
{
  QList<QModelIndex> selectedRows = getSelectedRows();
  OrderModel& orderModel = market->getOrderModel();

  bool hasMarket = market != 0;
  bool canCancel = getSelectedRows().size() > 0;
  bool canSubmit = false;

  if(hasMarket)
    foreach(const QModelIndex& index, selectedRows)
    {
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
