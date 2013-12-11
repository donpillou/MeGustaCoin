
#pragma once

class TransactionsWidget : public QWidget
{
  Q_OBJECT

public:
  TransactionsWidget(QWidget* parent, QSettings& settings, TransactionModel& transactionModel);

  void saveState(QSettings& settings);

private slots:
  void setMarket(Market* market);
  void updateToolBarButtons();

private:
  TransactionModel& transactionModel;
  QSettings& settings;
  Market* market;
  QTreeView* transactionView;
  QSortFilterProxyModel* proxyModel;

  QAction* refreshAction;
};

