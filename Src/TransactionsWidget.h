
#pragma once

class TransactionsWidget : public QWidget
{
  Q_OBJECT

public:
  TransactionsWidget(QWidget* parent, QSettings& settings);

private slots:
  void setMarket(Market* market);
  void updateToolBarButtons();

private:
  QSettings& settings;
  Market* market;
  QTreeView* transactionView;
  QSortFilterProxyModel* orderProxyModel;

  QAction* refreshAction;
};

