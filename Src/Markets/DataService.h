
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

  void createMarket(quint32 marketAdapterId, const QString& userName, const QString& key, const QString& secret);
  void removeMarket(quint32 id);
  void selectMarket(quint32 id);
  void refreshMarketOrders();
  void refreshMarketTransactions();
  void refreshMarketBalance();

  EBotMarketOrderDraft& createMarketOrderDraft(EBotMarketOrder::Type type, double price);
  void submitMarketOrderDraft(EBotMarketOrderDraft& draft);
  void cancelMarketOrder(EBotMarketOrder& order);
  void updateMarketOrder(EBotMarketOrder& order, double price, double amount);
  void removeMarketOrderDraft(EBotMarketOrderDraft& draft);

  void createSession(const QString& name, quint32 engineId, quint32 marketId);
  void removeSession(quint32 id);
  void stopSession(quint32 id);
  void startSessionSimulation(quint32 id);
  void startSession(quint32 id);
  void selectSession(quint32 id);

  EBotSessionItemDraft& createSessionItemDraft(EBotSessionItem::Type type, double flipPrice);
  void submitSessionItemDraft(EBotSessionItemDraft& draft);
  void updateSessionItem(EBotSessionItem& item, double flipPrice);
  void cancelSessionItem(EBotSessionItem& item);
  void removeSessionItemDraft(EBotSessionItemDraft& draft);

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
    virtual void receivedChannelInfo(quint32 channelId, const QString& channelName);
    virtual void receivedSubscribeResponse(quint32 channelId);
    virtual void receivedTrade(quint32 channelId, const meguco_trade_entity& trade);
    virtual void receivedTicker(quint32 channelId, const meguco_ticker_entity& ticker);
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

  quint32 getChannelId(const QString& channelName);

private slots:
  void handleEvents();
};
