
#pragma once

class MainWindow : public QTabFramework, public Entity::Listener
{
  Q_OBJECT

public:
  MainWindow();
  ~MainWindow();

private:
  QSettings settings;

  MarketsWidget* marketsWidget;
  OrdersWidget* ordersWidget;
  BotSessionsWidget* botSessionsWidget;
  BotTransactionsWidget* botTransactionsWidget;
  BotItemsWidget* botItemsWidget;
  BotOrdersWidget* botOrdersWidget;
  BotPropertiesWidget* botPropertiesWidget;
  BotLogWidget* botLogWidget;
  TransactionsWidget* transactionsWidget;
  //GraphWidget* graphWidget;
  LogWidget* logWidget;

  QMenu* viewMenu;

  Entity::Manager globalEntityManager;
  DataService dataService;
  BotService botService;
  QString selectedChannelName;

  struct ChannelData
  {
    QString channelName;
    TradesWidget* tradesWidget;
    GraphWidget* graphWidget;
    Entity::Manager channelEntityManager;

    ChannelData() : tradesWidget(0), graphWidget(0) {}
  };

  QHash<QString, ChannelData> channelDataMap;
  QMap<QString, GraphModel*> channelGraphModels;
  QSignalMapper liveTradesSignalMapper;
  QSignalMapper liveGraphSignalMapper;

private:
  void startDataService();
  void startBotService();
  void createChannelData(const QString& channelName, const QString& currencyBase, const QString currencyComm);
  ChannelData* getChannelData(const QString& channelName);
  void updateChannelSubscription(ChannelData& channelData);

private: // QMainWindow
  virtual void closeEvent(QCloseEvent* event);

private slots:
  //void login(); // todo: remove
  //void logout(); // todo: remove
  //void refresh();
  void updateWindowTitle();
  //void updateWindowTitleTicker();
  //void updateFocusPublicDataModel();
  void updateViewMenu();
  void about();
  void showOptions();
  void enableTradesUpdates(bool enable);
  void enableGraphUpdates(bool enable);
  void createLiveTradeWidget(const QString& channel);
  void createLiveGraphWidget(const QString& channel);

private: // Entity::Listener
  virtual void addedEntity(Entity& entity);
  virtual void updatedEntitiy(Entity& oldEntity, Entity& newEntity);
  virtual void removedAll(quint32 type);
};
