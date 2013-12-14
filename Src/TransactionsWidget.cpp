
#include "stdafx.h"

TransactionsWidget::TransactionsWidget(QWidget* parent, QSettings& settings, DataModel& dataModel) : QWidget(parent), dataModel(dataModel), transactionModel(dataModel.transactionModel)
{
  QToolBar* toolBar = new QToolBar(this);
  toolBar->setStyleSheet("QToolBar { border: 0px }");
  toolBar->setIconSize(QSize(16, 16));
  toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  refreshAction = toolBar->addAction(QIcon(":/Icons/arrow_refresh.png"), tr("&Refresh"));
  refreshAction->setEnabled(false);
  refreshAction->setShortcut(QKeySequence(QKeySequence::Refresh));
  connect(refreshAction, SIGNAL(triggered()), this, SLOT(refresh()));

  transactionView = new QTreeView(this);
  proxyModel = new QSortFilterProxyModel(this);
  proxyModel->setDynamicSortFilter(true);
  transactionView->setModel(proxyModel);
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

  proxyModel->setSourceModel(&transactionModel);
  QHeaderView* headerView = transactionView->header();
  headerView->resizeSection(0, 50);
  headerView->resizeSection(1, 110);
  headerView->resizeSection(2, 85);
  headerView->resizeSection(3, 100);
  headerView->resizeSection(4, 85);
  headerView->resizeSection(5, 75);
  headerView->resizeSection(6, 85);
  headerView->setStretchLastSection(false);
  headerView->setResizeMode(0, QHeaderView::Stretch);
  transactionView->sortByColumn(1);
  headerView->restoreState(settings.value("TransactionHeaderState").toByteArray());
}

void TransactionsWidget::saveState(QSettings& settings)
{
  settings.setValue("TransactionHeaderState", transactionView->header()->saveState());
}

void TransactionsWidget::setMarket(Market* market)
{
  this->market = market;
  updateToolBarButtons();
}

void TransactionsWidget::updateToolBarButtons()
{
  bool hasMarket = market != 0;

  refreshAction->setEnabled(hasMarket);
}

void TransactionsWidget::refresh()
{
  if(!market)
    return;
  dataModel.logModel.addMessage(LogModel::Type::information, "Refreshing transactions...");
  market->loadTransactions();
}
