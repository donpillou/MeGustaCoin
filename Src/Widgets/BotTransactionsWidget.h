
#pragma once

class BotTransactionsWidget : public QWidget
{
public:
  BotTransactionsWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager);

  void saveState(QSettings& settings);

private:
  QTabFramework& tabFramework;
  Entity::Manager& entityManager;

  SessionTransactionModel transactionModel;

  QTreeView* transactionView;
};
