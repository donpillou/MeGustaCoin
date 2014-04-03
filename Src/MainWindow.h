
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
  BotsWidget* botsWidget;
  TransactionsWidget* transactionsWidget;
  GraphWidget* graphWidget;
  LogWidget* logWidget;

  QMenu* viewMenu;

  DataModel dataModel;
  Entity::Manager botEntityManager;
  MarketService marketService;
  DataService dataService;
  BotService botService;

  struct ChannelData
  {
    TradesWidget* tradesWidget;
    GraphWidget* graphWidget;

    ChannelData() : tradesWidget(0), graphWidget(0) {}
  };

  QHash<QString, ChannelData> channelDataMap;
  QSignalMapper liveTradesSignalMapper;
  QSignalMapper liveGraphSignalMapper;

  void open(const QString& market, const QString& userName, const QString& key, const QString& secret);
  void startDataService();
  void startBotService();


  virtual void closeEvent(QCloseEvent* event);

private slots:
  void login();
  void logout();
  void refresh();
  void updateWindowTitle();
  void updateWindowTitleTicker();
  void updateFocusPublicDataModel();
  void updateViewMenu();
  void about();
  void showOptions();
  void enableTradesUpdates(bool enable);
  void enableGraphUpdates(bool enable);
  void createLiveTradeWidget(const QString& channel);
  void createLiveGraphWidget(const QString& channel);
};
