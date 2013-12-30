
#pragma once

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow();
  ~MainWindow();

private:
  QSettings settings;

  OrdersWidget* ordersWidget;
  TransactionsWidget* transactionsWidget;
  //TradesWidget* tradesWidget;
  //BookWidget* bookWidget;
  //GraphWidget* graphWidget;
  LogWidget* logWidget;

  DataModel dataModel;
  MarketService marketService;

  struct MarketData
  {
    MarketStreamService* streamService;
    PublicDataModel* publicDataModel;

    TradesWidget* tradesWidget;
    QDockWidget* tradesDockWidget;
    BookWidget* bookWidget;
    QDockWidget* bookDockWidget;
    GraphWidget* graphWidget;
    QDockWidget* graphDockWidget;

    enum class EnabledWidgets
    {
      trades = 0x01,
      book = 0x02,
      graph = 0x04,
    };

    int enabledWidgets;

    MarketData() : enabledWidgets(0) {}
  };

  QList<MarketData> marketDataList;
  QMap<QString, PublicDataModel*> publicDataModels;

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
