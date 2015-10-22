
#pragma once

class DataService : public QObject
{
  Q_OBJECT

public:
  DataService(Entity::Manager& globalEntityManager);
  ~DataService();

  void start(const QString& server, const QString& userName, const QString& password);
  void stop();

  void subscribe(const QString& channel, Entity::Manager& entityManager);
  void unsubscribe(const QString& channel);
  Entity::Manager* getSubscription(const QString& channel);

  void createBroker(quint64 marketId, const QString& userName, const QString& key, const QString& secret);
  void removeBroker(quint32 brokerId);
  void selectBroker(quint32 brokerId);
  void refreshBrokerOrders();
  void refreshBrokerTransactions();
  void refreshBrokerBalance();

  EBotMarketOrderDraft& createBrokerOrderDraft(EBotMarketOrder::Type type, double price);
  void removeBrokerOrderDraft(EBotMarketOrderDraft &draft);
  void submitBrokerOrderDraft(EBotMarketOrderDraft& draft);
  void cancelBrokerOrder(EBotMarketOrder& order);
  void updateBrokerOrder(EBotMarketOrder& order, double price, double amount);
  void removeBrokerOrder(EBotMarketOrder& order);

  void createSession(const QString& name, quint64 engineId, quint64 marketId);
  void removeSession(quint32 sessionId);
  void selectSession(quint32 sessionId);
  void stopSession(quint32 sessionId);
  void startSessionSimulation(quint32 sessionId);
  void startSession(quint32 sessionId);

  EBotSessionItemDraft& createSessionAssetDraft(EBotSessionItem::Type type, double flipPrice);
  void submitSessionAssetDraft(EBotSessionItemDraft& draft);
  void updateSessionAsset(EBotSessionItem& asset, double flipPrice);
  void removeSessionAsset(EBotSessionItem& asset);
  void removeSessionAssetDraft(EBotSessionItemDraft& draft);

  void updateSessionProperty(EBotSessionProperty& property, const QString& value);

private:
  class WorkerThread;

  class Job
  {
  public:
    virtual ~Job() {}
    virtual bool execute(WorkerThread& workerThread) = 0;
  };

  class Event
  {
  public:
    virtual ~Event() {}
    virtual void handle(DataService& dataService) = 0;
  };

  class WorkerThread : public QThread, public DataConnection::Callback
  {
  public:
    class SubscriptionData
    {
    public:
      QString channelName;
      EDataTradeData* eTradeData;

    public:
      SubscriptionData() : eTradeData(0) {}
      ~SubscriptionData() {delete eTradeData;}
    };

  public:
    WorkerThread(DataService& dataService, JobQueue<Event*>& eventQueue, JobQueue<Job*>& jobQueue, const QString& server, const QString& userName, const QString& password) :
      dataService(dataService), eventQueue(eventQueue), jobQueue(jobQueue), canceled(false), 
      server(server), userName(userName), password(password) {}

    const QString& getServer() const {return server;}
    const QString& getUserName() const {return userName;}
    const QString& getPassword() const {return password;}
    void interrupt();

    void addMessage(ELogMessage::Type type, const QString& message);
    void delegateEntity(Entity* entity);
    void removeEntity(EType type, quint64 id);

  public:
    DataService& dataService;
    JobQueue<Event*>& eventQueue;
    JobQueue<Job*>& jobQueue;
    DataConnection connection;
    bool canceled;
    QString server;
    QString userName;
    QString password;
    QHash<quint32, SubscriptionData> subscriptionData;

  private:
    void setState(EDataService::State state);
    void process();

  private: // QThread
    virtual void run();

  private: // DataConnection::Callback
    virtual void receivedMarket(quint32 tableId, const QString& channelName);
    virtual void receivedBroker(quint32 brokerId, const meguco_user_broker_entity& broker, const QString& userName);
    virtual void removedBroker(quint32 brokerId);
    virtual void receivedSession(quint32 sessionId, const QString& name, const meguco_user_session_entity& session);
    virtual void removedSession(quint32 sessionId);
    virtual void receivedTrade(quint32 tableId, const meguco_trade_entity& trade, qint64 timeOffset);
    virtual void receivedTicker(quint32 tableId, const meguco_ticker_entity& ticker);
    virtual void receivedBrokerType(const meguco_broker_type_entity& brokerType, const QString& name);
    virtual void receivedBotType(const meguco_bot_type_entity& botType, const QString& name);
    virtual void receivedBrokerBalance(const meguco_user_broker_balance_entity& balance);
    virtual void receivedBrokerOrder(const meguco_user_broker_order_entity& order);
    virtual void receivedBrokerTransaction(const meguco_user_broker_transaction_entity& transaction);
    virtual void receivedBrokerLog(const meguco_log_entity& log, const QString& message);
    virtual void receivedSessionOrder(const meguco_user_broker_order_entity& order);
    virtual void receivedSessionTransaction(const meguco_user_broker_transaction_entity& transaction);
    virtual void receivedSessionAsset(const meguco_user_session_asset_entity& asset);
    virtual void receivedSessionLog(const meguco_log_entity& log, const QString& message);
    virtual void receivedSessionProperty(const meguco_user_session_property_entity& property, const QString& name, const QString& value, const QString& unit);
    virtual void receivedProcess(const meguco_process_entity& process, const QString& cmd);
    virtual void removedProcess(quint64 processId);
  };

private:
  Entity::Manager& globalEntityManager;
  WorkerThread* thread;

  JobQueue<Event*> eventQueue;
  JobQueue<Job*> jobQueue;
  QHash<quint32, Entity::Manager*> activeSubscriptions;
  bool isConnected;
  QHash<QString, Entity::Manager*> subscriptions;

private:
  void addLogMessage(ELogMessage::Type type, const QString& message);
  void controlBroker(meguco_user_broker_control_code code);
  void controlBrokerOrder(quint64 orderId, meguco_user_broker_order_control_code code, const void* data, size_t size);
  void controlSession(quint32 sessionId, meguco_user_session_control_code code);

  quint32 getChannelId(const QString& channelName);

private slots:
  void handleEvents();
};
