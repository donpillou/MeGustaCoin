
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
    TransactionModel& transactionModel = market->getTransactionModel();

    orderProxyModel->setSourceModel(&transactionModel);
    QHeaderView* headerView = transactionView->header();
    headerView->resizeSection(0, 35);
    headerView->resizeSection(1, 110);
    headerView->resizeSection(2, 85);
    headerView->resizeSection(3, 100);
    headerView->resizeSection(4, 85);
    headerView->resizeSection(5, 75);
    headerView->resizeSection(6, 85);
    transactionView->sortByColumn(1);
    headerView->restoreState(settings.value("TransactionHeaderState").toByteArray());
  }
  updateToolBarButtons();
}

void TransactionsWidget::updateToolBarButtons()
{
  bool hasMarket = market != 0;

  refreshAction->setEnabled(hasMarket);
}
