
#include "stdafx.h"

BotTransactionsWidget::BotTransactionsWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager) :
  QWidget(&tabFramework), tabFramework(tabFramework), entityManager(entityManager),  transactionModel(entityManager)
{
  setWindowTitle(tr("Bot Transactions"));

  transactionView = new QTreeView(this);
  transactionView->setUniformRowHeights(true);
  SessionTransactionSortProxyModel* transactionProxyModel = new SessionTransactionSortProxyModel(this);
  transactionProxyModel->setDynamicSortFilter(true);
  transactionProxyModel->setSourceModel(&transactionModel);
  transactionView->setModel(transactionProxyModel);
  transactionView->setSortingEnabled(true);
  transactionView->setRootIsDecorated(false);
  transactionView->setAlternatingRowColors(true);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->addWidget(transactionView);
  setLayout(layout);

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
  settings.beginGroup("BotTransactions");
  headerView->restoreState(settings.value("HeaderState").toByteArray());
  settings.endGroup();
}


void BotTransactionsWidget::saveState(QSettings& settings)
{
  settings.beginGroup("BotTransactions");
  settings.setValue("HeaderState", transactionView->header()->saveState());
  settings.endGroup();
}
