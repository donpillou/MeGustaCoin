
#pragma once

class BotTransactionsWidget : public QWidget
{
public:
  BotTransactionsWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager);

  void saveState(QSettings& settings);

private:
  SessionTransactionModel transactionModel;
  QTreeView* transactionView;
};
