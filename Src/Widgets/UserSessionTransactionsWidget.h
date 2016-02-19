
#pragma once

class UserSessionTransactionsWidget : public QWidget
{
public:
  UserSessionTransactionsWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager);

  void saveState(QSettings& settings);

private:
  UserSessionTransactionsModel transactionsModel;
  QTreeView* transactionView;
};
