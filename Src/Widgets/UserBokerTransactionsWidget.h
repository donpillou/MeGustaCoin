
#pragma once

class UserBokerTransactionsWidget : public QWidget
{
public:
  UserBokerTransactionsWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager);

  void saveState(QSettings& settings);

private:
  UserSessionTransactionsModel transactionsModel;
  QTreeView* transactionView;
};
