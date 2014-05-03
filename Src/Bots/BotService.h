
#pragma once

class BotService : public QObject
{
  Q_OBJECT

public:
  BotService(DataModel& dataModel, Entity::Manager& entityManager);
  ~BotService();

  void start(const QString& server, const QString& userName, const QString& password);
  void stop();

  void createMarket(quint32 marketAdapterId, const QString& userName, const QString& key, const QString& secret);
  void removeMarket(quint32 id);
  void selectMarket(quint32 id);
  void refreshMarketOrders();
  void refreshMarketTransactions();

  EBotMarketOrderDraft& createMarketOrderDraft(EBotMarketOrder::Type type, double price);
  void submitMarketOrderDraft(EBotMarketOrderDraft& draft);
  void cancelMarketOrder(EBotMarketOrder& order);
  void removeMarketOrderDraft(EBotMarketOrderDraft& draft);

  void createSession(const QString& name, quint32 engineId, quint32 marketId, double balanceBase, double balanceComm);
  void removeSession(quint32 id);
  void startSessionSimulation(quint32 id);
  void stopSession(quint32 id);
  void selectSession(quint32 id);

  bool isConnected() const {return connected;}

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
    void addMessage(LogModel::Type type, const QString& message);
    void setState(EBotService::State state);
    void process();

  private: // QThread
    virtual void run();

  private: // BotConnection::Callback
    virtual void receivedUpdateEntity(BotProtocol::Entity& entity, size_t size);
    virtual void receivedRemoveEntity(const BotProtocol::Entity& entity);
    virtual void receivedControlEntityResponse(BotProtocol::Entity& entity, size_t size);
    virtual void receivedCreateEntityResponse(const BotProtocol::CreateResponse& entity);
  };

private:
  DataModel& dataModel;
  Entity::Manager& entityManager;
  WorkerThread* thread;

  JobQueue<Event*> eventQueue;
  JobQueue<Job*> jobQueue;
  bool connected;

private:
  void createEntity(const void* args, size_t size);
  void removeEntity(BotProtocol::EntityType type, quint32 id);
  void controlEntity(const void* args, size_t size);

  template<size_t N> void setString(char(&str)[N], const QString& value)
  {
    QByteArray buf = value.toUtf8();
    size_t size = buf.length() + 1;
    if(size > N - 1)
      size = N - 1;
    memcpy(str, buf.constData(), size);
    str[N - 1] = '\0';
  }

private slots:
  void handleEvents();
};
