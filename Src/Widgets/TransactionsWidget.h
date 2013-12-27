
#pragma once

class TransactionsWidget : public QWidget
{
  Q_OBJECT

public:
  TransactionsWidget(QWidget* parent, QSettings& settings, DataModel& dataModel);

  void saveState(QSettings& settings);

public slots:
  void refresh();

private slots:
  void setMarket(Market* market);
  void updateToolBarButtons();

private:
  DataModel& dataModel;
  TransactionModel& transactionModel;
  Market* market;
  QTreeView* transactionView;
  QSortFilterProxyModel* proxyModel;

  QAction* refreshAction;
};

