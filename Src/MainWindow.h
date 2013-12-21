
#pragma once

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow();
  ~MainWindow();

signals:
  void marketChanged(Market* market);

private:
  QSettings settings;

  OrdersWidget* ordersWidget;
  TransactionsWidget* transactionsWidget;
  TradesWidget* tradesWidget;
  BookWidget* bookWidget;
  GraphWidget* graphWidget;
  LogWidget* logWidget;

  DataModel dataModel;
  Market* market;
  QString marketName;
  QString userName;

  bool liveTradeUpdatesEnabled;
  bool orderBookUpdatesEnabled;
  bool graphUpdatesEnabled;

  void open(const QString& market, const QString& userName, const QString& key, const QString& secret);

  virtual void closeEvent(QCloseEvent* event);

private slots:
  void login();
  void logout();
  void refresh();
  void updateWindowTitle();
  void about();
  void enableLiveTradesUpdates(bool enable);
  void enableOrderBookUpdates(bool enable);
  void enableGraphUpdates(bool enable);
};
