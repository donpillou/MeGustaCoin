
#pragma once

class BotService : public QObject
{
  Q_OBJECT

public:
  BotService(DataModel& dataModel);
  ~BotService();

  void start(const QString& server, const QString& userName, const QString& password);
  void stop();

  void createSession(const QString& name, const QString& engine);

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
    void setState(BotsModel::State state);
    void process();

  private: // QThread
    virtual void run();

  private: // BotConnection::Callback
    virtual void receivedErrorResponse(const QString& errorMessage);
    virtual void receivedEngine(const QString& engine);
    virtual void receivedSession(quint32 id, const QString& name, const QString& engine);
  };

  DataModel& dataModel;
  WorkerThread* thread;

  JobQueue<Event*> eventQueue;
  JobQueue<Job*> jobQueue;
  bool connected;

private slots:
  void handleEvents();
};
