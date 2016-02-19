
#include "stdafx.h"

UserBrokerTransactionsWidget::UserBrokerTransactionsWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager) :
  QWidget(&tabFramework), transactionsModel(entityManager)
{
  setWindowTitle(tr("Bot Transactions"));

  transactionView = new QTreeView(this);
  transactionView->setUniformRowHeights(true);
  UserSessionTransactionsSortProxyModel* transactionsProxyModel = new UserSessionTransactionsSortProxyModel(this);
  transactionsProxyModel->setDynamicSortFilter(true);
  transactionsProxyModel->setSourceModel(&transactionsModel);
  transactionView->setModel(transactionsProxyModel);
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
  transactionView->sortByColumn(1);
  settings.beginGroup("BotTransactions");
  headerView->restoreState(settings.value("HeaderState").toByteArray());
  settings.endGroup();
  headerView->setStretchLastSection(false);
  headerView->setResizeMode(0, QHeaderView::Stretch);
}

void UserBrokerTransactionsWidget::saveState(QSettings& settings)
{
  settings.beginGroup("BotTransactions");
  settings.setValue("HeaderState", transactionView->header()->saveState());
  settings.endGroup();
}
