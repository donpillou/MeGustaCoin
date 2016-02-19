
#pragma once

class UserBrokerTransactionsWidget : public QWidget
{
public:
  UserBrokerTransactionsWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager);

  void saveState(QSettings& settings);

private:
  UserSessionTransactionsModel transactionsModel;
  QTreeView* transactionView;
};
