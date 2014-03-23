
#pragma once

class BotService : public QObject
{
  Q_OBJECT

public:
  BotService(DataModel& dataModel);
  ~BotService();

  void start(const QString& server, const QString& userName, const QString& password);
  void stop();

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
    //virtual void receivedChannelInfo(const QString& channelName);
    //virtual void receivedSubscribeResponse(const QString& channelName, quint64 channelId);
    //virtual void receivedUnsubscribeResponse(const QString& channelName, quint64 channelId);
    //virtual void receivedTrade(quint64 channelId, const DataProtocol::Trade& trade);
    //virtual void receivedTicker(quint64 channelId, const DataProtocol::Ticker& ticker);
    //virtual void receivedErrorResponse(const QString& message);
  };

  DataModel& dataModel;
  WorkerThread* thread;

  JobQueue<Event*> eventQueue;
  JobQueue<Job*> jobQueue;
  bool connected;

private slots:
  void handleEvents();
};
