
#include "stdafx.h"

TransactionsWidget::TransactionsWidget(QWidget* parent, QSettings& settings) : QWidget(parent), settings(settings)
{
  QToolBar* toolBar = new QToolBar(this);
  toolBar->setStyleSheet("QToolBar { border: 0px }");
  toolBar->setIconSize(QSize(16, 16));
  toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  refreshAction = toolBar->addAction(QIcon(":/Icons/arrow_refresh.png"), tr("&Refresh"));
  refreshAction->setEnabled(false);
  refreshAction->setShortcut(QKeySequence(QKeySequence::Refresh));
  connect(refreshAction, SIGNAL(triggered()), parent, SLOT(refresh()));

  transactionView = new QTreeView(this);
  orderProxyModel = new QSortFilterProxyModel(this);
  orderProxyModel->setDynamicSortFilter(true);
  transactionView->setModel(orderProxyModel);
  transactionView->setSortingEnabled(true);
  transactionView->setRootIsDecorated(false);
  transactionView->setAlternatingRowColors(true);
  //transactionView->setSelectionMode(QAbstractItemView::ExtendedSelection);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->addWidget(toolBar);
  layout->addWidget(transactionView);
  setLayout(layout);
}

void TransactionsWidget::setMarket(Market* market)
{
  this->market = market;
  if(!market)
  {
    settings.setValue("TransactionHeaderState", transactionView->header()->saveState());
    orderProxyModel->setSourceModel(0);
  }
  else
  {
    OrderModel& orderModel = market->getOrderModel();
    connect(&orderModel, SIGNAL(orderEdited(const QModelIndex&)), this, SLOT(updateOrder(const QModelIndex&)));

    orderProxyModel->setSourceModel(&orderModel);
    transactionView->header()->resizeSection(0, 85);
    transactionView->header()->resizeSection(1, 85);
    transactionView->header()->resizeSection(2, 150);
    transactionView->header()->resizeSection(3, 85);
    transactionView->header()->resizeSection(4, 85);
    transactionView->header()->resizeSection(5, 85);
    transactionView->header()->restoreState(settings.value("TransactionHeaderState").toByteArray());
  }
  updateToolBarButtons();
}

void TransactionsWidget::updateToolBarButtons()
{
  bool hasMarket = market != 0;

  refreshAction->setEnabled(hasMarket);
}
