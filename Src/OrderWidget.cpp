
#include "stdafx.h"

OrderWidget::OrderWidget(QWidget* parent, QSettings& settings) : QWidget(parent), settings(settings)
{
  QToolBar* toolBar = new QToolBar(this);
  toolBar->setStyleSheet("QToolBar { border: 0px }");
  toolBar->setIconSize(QSize(16, 16));
  toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  QAction* action = toolBar->addAction(QIcon(":/Icons/arrow_refresh.png"), tr("&Refresh"));
  action->setShortcut(QKeySequence(QKeySequence::Refresh));
  connect(action, SIGNAL(triggered()), parent, SLOT(refresh()));
  /*
  action = toolBar->addAction(QIcon(":/Icons/bitcoin_add.png"), tr("&Buy"));
  action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_B));
  connect(action, SIGNAL(triggered()), this, SLOT(newBuyOrder()));
  action = toolBar->addAction(QIcon(":/Icons/money_add.png"), tr("&Sell"));
  action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_S));
  connect(action, SIGNAL(triggered()), this, SLOT(newSellOrder()));

  action = toolBar->addAction(QIcon(":/Icons/bullet_go.png"), tr("S&ubmit"));
  action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_U));
  connect(action, SIGNAL(triggered()), this, SLOT(submitOrder()));

  action = toolBar->addAction(QIcon(":/Icons/cancel.png"), tr("&Cancel"));
  action->setShortcut(QKeySequence(Qt::Key_Delete));
  connect(action, SIGNAL(triggered()), this, SLOT(cancelOrder()));
  */

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

void OrderWidget::setMarket(Market* market)
{
  this->market = market;
  if(!market)
  {
    settings.setValue("OrderHeaderState", orderView->header()->saveState());
    orderProxyModel->setSourceModel(0);
  }
  else
  {
    orderProxyModel->setSourceModel(&market->getOrderModel());
    orderView->header()->resizeSection(0, 85);
    orderView->header()->resizeSection(1, 85);
    orderView->header()->resizeSection(2, 150);
    orderView->header()->resizeSection(3, 85);
    orderView->header()->resizeSection(4, 85);
    orderView->header()->resizeSection(5, 85);
    orderView->header()->restoreState(settings.value("OrderHeaderState").toByteArray());
  }
}

void OrderWidget::newBuyOrder()
{
  addOrder(OrderModel::Order::Type::buy);
}

void OrderWidget::newSellOrder()
{
  addOrder(OrderModel::Order::Type::sell);
}

void OrderWidget::addOrder(OrderModel::Order::Type type)
{
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

void OrderWidget::submitOrder()
{
  QItemSelectionModel* selModel = orderView->selectionModel();
  QList<QModelIndex> seletedIndices = selModel->selectedRows();
  
  QTypeInfo<QModelIndex> asds;
  int k = 32;
  //seletedIndices.clear();


  /*
  OrderModel& orderModel = market->getOrderModel();
  foreach(const QModelIndex& proxyIndex, seletedIndices)
  {
    QModelIndex index = orderProxyModel->mapToSource(proxyIndex);
    const OrderModel::Order* order = orderModel.getOrder(index);
    if(order->state == OrderModel::Order::State::draft)
    {
      //market->create
      // todo
    }
  }
  */
}

void OrderWidget::cancelOrder()
{
}

