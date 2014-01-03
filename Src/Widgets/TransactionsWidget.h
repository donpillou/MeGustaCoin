
#pragma once

class TransactionsWidget : public QWidget
{
  Q_OBJECT

public:
  TransactionsWidget(QWidget* parent, QSettings& settings, DataModel& dataModel, MarketService& marketService);

  void saveState(QSettings& settings);

public slots:
  void refresh();

private slots:
  void updateToolBarButtons();
  void updateTitle();

private:
  DataModel& dataModel;
  TransactionModel& transactionModel;
  MarketService& marketService;

  QTreeView* transactionView;
  QSortFilterProxyModel* proxyModel;

  QAction* refreshAction;
};

