
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

  EUserBrokerOrderDraft& createBrokerOrderDraft(EUserBrokerOrder::Type type, double price);
  void removeBrokerOrderDraft(EUserBrokerOrderDraft &draft);
  void submitBrokerOrderDraft(EUserBrokerOrderDraft& draft);
  void cancelBrokerOrder(EUserBrokerOrder& order);
  void updateBrokerOrder(EUserBrokerOrder& order, double price, double amount);
  void removeBrokerOrder(EUserBrokerOrder& order);

  void createSession(const QString& name, quint64 engineId, quint64 marketId);
  void removeSession(quint32 sessionId);
  void selectSession(quint32 sessionId);
  void stopSession(quint32 sessionId);
  void startSession(quint32 sessionId, meguco_user_session_mode mode);

  EUserSessionAssetDraft& createSessionAssetDraft(EUserSessionAsset::Type type, double flipPrice);
  void submitSessionAssetDraft(EUserSessionAssetDraft& draft);
  void updateSessionAsset(EUserSessionAsset& asset, double flipPrice);
  void removeSessionAsset(EUserSessionAsset& asset);
  void removeSessionAssetDraft(EUserSessionAssetDraft& draft);

  void updateSessionProperty(EUserSessionProperty& property, const QString& value);

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
      EMarketTickerData* eTickerData;

    public:
      SubscriptionData() : eTradeData(0), eTickerData(0) {}
      ~SubscriptionData() {delete eTradeData; delete eTickerData;}
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
    void clearEntities(EType type);

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
    void setState(EConnection::State state);
    void process();

  private: // QThread
    virtual void run();

  private: // DataConnection::Callback
    virtual void receivedBrokerType(const meguco_broker_type_entity& brokerType, const QString& name);
    virtual void receivedBotType(const meguco_bot_type_entity& botType, const QString& name);

    virtual void receivedMarket(quint32 marketId, const QString& channelName);
    virtual void receivedBroker(quint32 brokerId, const meguco_user_broker_entity& broker, const QString& userName);
    virtual void removedBroker(quint32 brokerId);
    virtual void receivedSession(quint32 sessionId, const QString& name, const meguco_user_session_entity& session);
    virtual void removedSession(quint32 sessionId);

    virtual void receivedMarketTrade(quint32 marketId, const meguco_trade_entity& trade, qint64 timeOffset);
    virtual void receivedMarketTicker(quint32 marketId, const meguco_ticker_entity& ticker);

    virtual void receivedBrokerBalance(const meguco_user_broker_balance_entity& balance);
    virtual void removedBrokerBalance(quint64 balanceId);
    virtual void clearBrokerBalance();
    virtual void receivedBrokerOrder(const meguco_user_broker_order_entity& order);
    virtual void removedBrokerOrder(quint64 orderId);
    virtual void clearBrokerOrders();
    virtual void receivedBrokerTransaction(const meguco_user_broker_transaction_entity& transaction);
    virtual void removedBrokerTransaction(quint64 transactionId);
    virtual void clearBrokerTransactions();
    virtual void receivedBrokerLog(const meguco_log_entity& log, const QString& message);
    virtual void removedBrokerLog(quint64 logId) {}
    virtual void clearBrokerLog() {}

    virtual void receivedSessionOrder(const meguco_user_broker_order_entity& order);
    virtual void removedSessionOrder(quint64 orderId);
    virtual void clearSessionOrders();
    virtual void receivedSessionTransaction(const meguco_user_broker_transaction_entity& transaction);
    virtual void removedSessionTransaction(quint64 transactionId);
    virtual void clearSessionTransactions();
    virtual void receivedSessionAsset(const meguco_user_session_asset_entity& asset);
    virtual void removedSessionAsset(quint64 assertId);
    virtual void clearSessionAssets();
    virtual void receivedSessionLog(const meguco_log_entity& log, const QString& message);
    virtual void removedSessionLog(quint64 logId);
    virtual void clearSessionLog();
    virtual void receivedSessionProperty(const meguco_user_session_property_entity& property, const QString& name, const QString& value, const QString& unit);
    virtual void removedSessionProperty(quint64 propertyId);
    virtual void clearSessionProperties();

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
  //void controlBroker(meguco_user_broker_control_code code);
  //void controlBrokerOrder(quint64 orderId, meguco_user_broker_order_control_code code, const void* data, size_t size);
  //void controlSession(quint32 sessionId, meguco_user_session_control_code code); // todo: ??

  quint32 getChannelId(const QString& channelName);

private slots:
  void handleEvents();
};
