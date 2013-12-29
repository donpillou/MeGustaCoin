
#include "stdafx.h"

OrdersWidget::OrdersWidget(QWidget* parent, QSettings& settings, DataModel& dataModel, MarketService& marketService) : QWidget(parent), dataModel(dataModel), orderModel(dataModel.orderModel), marketService(marketService)
{
  connect(&dataModel, SIGNAL(changedMarket()), this, SLOT(updateToolBarButtons()));

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

  class OrderSortProxyModel : public QSortFilterProxyModel
  {
  public:
    OrderSortProxyModel(QObject* parent, OrderModel& orderModel) : QSortFilterProxyModel(parent), orderModel(orderModel) {}

  private:
    OrderModel& orderModel;

    virtual bool lessThan(const QModelIndex& left, const QModelIndex& right) const
    {
      const OrderModel::Order* leftOrder = orderModel.getOrder(left);
      const OrderModel::Order* rightOrder = orderModel.getOrder(right);
      switch((OrderModel::Column)left.column())
      {
      case OrderModel::Column::date:
        return leftOrder->date.msecsTo(rightOrder->date) > 0;
      case OrderModel::Column::value:
        return leftOrder->amount * leftOrder->price < rightOrder->amount * rightOrder->price;
      case OrderModel::Column::amount:
        return leftOrder->amount < rightOrder->amount;
      case OrderModel::Column::price:
        return leftOrder->price < rightOrder->price;
      case OrderModel::Column::total:
        return leftOrder->total < rightOrder->total;
      }
      return QSortFilterProxyModel::lessThan(left, right);
    }
  };

  orderView = new QTreeView(this);
  proxyModel = new OrderSortProxyModel(this, orderModel);
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
  double price = 0;
  const Market::TickerData& tickerData = dataModel.getTickerData();
  if(tickerData.highestBuyOrder != 0. && tickerData.lowestSellOrder != 0.)
    price = type == OrderModel::Order::Type::buy ? (tickerData.highestBuyOrder + 0.01) : (tickerData.lowestSellOrder - 0.01);

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
      /*
      double maxAmount = order->type == OrderModel::Order::Type::buy ? marketService.getMaxBuyAmout(price) : marketService.getMaxSellAmout();
      if(amount > maxAmount)
      {
        orderModel.setOrderNewAmount(order->id, maxAmount);
        amount = maxAmount;
      }
      */

      marketService.createOrder(order->id, order->type == OrderModel::Order::Type::buy ? amount : -amount, price);
    }
  }
  updateToolBarButtons();
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
        marketService.cancelOrder(order->id);
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
  updateToolBarButtons();
}

void OrdersWidget::updateOrder(const QModelIndex& index)
{
  const OrderModel::Order* order = orderModel.getOrder(index);
  if(order->state != OrderModel::Order::State::open || (order->newAmount == 0. && order->newPrice == 0.))
    return;
  double amount = order->newAmount != 0. ? order->newAmount : order->amount;
  double price = order->newPrice != 0. ? order->newPrice : order->price;

  /*
  double maxAmount = order->type == OrderModel::Order::Type::buy ? marketService.getMaxBuyAmout(price, order->amount, order->price) : marketService.getMaxSellAmout() + order->amount;
  if(amount > maxAmount)
  {
    orderModel.setOrderNewAmount(order->id, maxAmount);
    amount = maxAmount;
  }
  */

  marketService.updateOrder(order->id, order->type == OrderModel::Order::Type::buy ? amount : -amount, price);
  updateToolBarButtons();
}

void OrdersWidget::updateToolBarButtons()
{
  QList<QModelIndex> selectedRows = getSelectedRows();

  bool hasMarket = marketService.isReady();
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
  submitAction->setEnabled(hasMarket && canSubmit);
  cancelAction->setEnabled(canCancel);
}

void OrdersWidget::refresh()
{
  marketService.loadOrders();
}