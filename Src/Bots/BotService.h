
#pragma once

class BotService : public QObject
{
  Q_OBJECT

public:
  BotService(Entity::Manager& entityManager);
  ~BotService();

  void start(const QString& server, const QString& userName, const QString& password);
  void stop();

  void createMarket(quint32 marketAdapterId, const QString& userName, const QString& key, const QString& secret);
  void removeMarket(quint32 id);
  void selectMarket(quint32 id);
  void refreshMarketOrders();
  void refreshMarketTransactions();
  void refreshMarketBalance();

  EBotMarketOrderDraft& createMarketOrderDraft(EBotMarketOrder::Type type, double price);
  void submitMarketOrderDraft(EBotMarketOrderDraft& draft);
  void updateMarketOrder(EBotMarketOrder& order, double price, double amount);
  void cancelMarketOrder(EBotMarketOrder& order);
  void removeMarketOrderDraft(EBotMarketOrderDraft& draft);

  void createSession(const QString& name, quint32 engineId, quint32 marketId, double balanceBase, double balanceComm);
  void removeSession(quint32 id);
  void startSessionSimulation(quint32 id);
  void startSession(quint32 id);
  void stopSession(quint32 id);
  void selectSession(quint32 id);

  EBotSessionItemDraft& createSessionItemDraft(EBotSessionItem::Type type, double flipPrice);
  void submitSessionItemDraft(EBotSessionItemDraft& draft);

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
    virtual void handle(BotService& botService) = 0;
  };

  class WorkerThread : public QThread, public BotConnection::Callback
  {
  public:
    WorkerThread(BotService& botService, JobQueue<Event*>& eventQueue, JobQueue<Job*>& jobQueue,
      const QString& server, const QString& userName, const QString& password) :
      botService(botService), eventQueue(eventQueue), jobQueue(jobQueue), canceled(false),
      server(server), userName(userName), password(password) {}

    const QString& getServer() const {return server;}
    const QString& getUserName() const {return userName;}
    const QString& getPassword() const {return password;}

    void interrupt();

  public:
    BotService& botService;
    JobQueue<Event*>& eventQueue;
    JobQueue<Job*>& jobQueue;
    BotConnection connection;
    bool canceled;

    QString server;
    QString userName;
    QString password;

  private:
    void addMessage(ELogMessage::Type type, const QString& message);
    void setState(EBotService::State state);
    void process();
    static EType getEType(BotProtocol::EntityType entityType);
    static Entity* createEntity(BotProtocol::Entity& data, size_t size);

  private: // QThread
    virtual void run();

  private: // BotConnection::Callback
    virtual void receivedUpdateEntity(BotProtocol::Entity& entity, size_t size);
    virtual void receivedRemoveEntity(const BotProtocol::Entity& entity);
    virtual void receivedRemoveAllEntities(const BotProtocol::Entity& entity);
    virtual void receivedControlEntityResponse(quint32 requestId, BotProtocol::Entity& entity, size_t size);
    virtual void receivedCreateEntityResponse(quint32 requestId, BotProtocol::Entity& entity, size_t size);
    virtual void receivedErrorResponse(quint32 requestId, BotProtocol::ErrorResponse& response);
  };

private:
  Entity::Manager& entityManager;
  WorkerThread* thread;

  JobQueue<Event*> eventQueue;
  JobQueue<Job*> jobQueue;

private:
  void createEntity(quint32 requestId, const void* args, size_t size);
  void updateEntity(quint32 requestId, const void* args, size_t size);
  void removeEntity(quint32 requestId, BotProtocol::EntityType type, quint32 id);
  void controlEntity(quint32 requestId, const void* args, size_t size);

  void addLogMessage(ELogMessage::Type type, const QString& message);

private slots:
  void handleEvents();
};
