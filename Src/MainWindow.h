
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
  LogWidget* logWidget;
  ProcessesWidget* processesWidget;

  QMenu* viewMenu;

  Entity::Manager globalEntityManager;
  DataService dataService;
  GraphService graphService;
  QString selectedChannelName;

  struct ChannelData
  {
    TradesWidget* tradesWidget;
    GraphWidget* graphWidget;
    Entity::Manager* channelEntityManager;
    GraphModel* graphModel;

    ChannelData() : tradesWidget(0), graphWidget(0), channelEntityManager(0), graphModel(0) {}
    ~ChannelData()
    {
      delete graphModel;
      delete channelEntityManager;
    }
  };

  QHash<QString, ChannelData> channelDataMap;
  QSignalMapper liveTradesSignalMapper;
  QSignalMapper liveGraphSignalMapper;

private:
  void startDataService();
  void createChannelData(const QString& channelName, const QString& currencyBase, const QString currencyComm);
  ChannelData* getChannelData(const QString& channelName);
  void updateChannelSubscription(ChannelData& channelData, bool enable);

private: // QMainWindow
  virtual void closeEvent(QCloseEvent* event);

private slots:
  void updateWindowTitle();
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
