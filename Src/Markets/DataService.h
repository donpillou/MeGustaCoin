
#pragma once

class DataService : public QObject
{
  Q_OBJECT

public:
  DataService(DataModel& dataModel);
  ~DataService();

  void start(const QString& server);
  void stop();

  void subscribe(const QString& channel);
  void unsubscribe(const QString& channel);

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
    WorkerThread(DataService& dataService, JobQueue<Event*>& eventQueue, JobQueue<Job*>& jobQueue, const QString& server) :
      dataService(dataService), eventQueue(eventQueue), jobQueue(jobQueue), canceled(false), 
      server(server) {}

    const QString& getServer() const {return server;}
    void interrupt();

  public:
    DataService& dataService;
    JobQueue<Event*>& eventQueue;
    JobQueue<Job*>& jobQueue;
    DataConnection connection;
    bool canceled;
    QString server;
    QHash<quint64, QList<DataProtocol::Trade> > replayedTrades;

  private:
    void addMessage(LogModel::Type type, const QString& message);
    void setState(PublicDataModel::State state);
    void process();

  private: // QThread
    virtual void run();

  private: // DataConnection::Callback
    virtual void receivedChannelInfo(const QString& channelName);
    virtual void receivedSubscribeResponse(const QString& channelName, quint64 channelId);
    virtual void receivedUnsubscribeResponse(const QString& channelName, quint64 channelId);
    virtual void receivedTrade(quint64 channelId, const DataProtocol::Trade& trade);
    virtual void receivedTicker(quint64 channelId, const DataProtocol::Ticker& ticker);
    virtual void receivedErrorResponse(const QString& message);
  };

  DataModel& dataModel;
  WorkerThread* thread;

  JobQueue<Event*> eventQueue;
  JobQueue<Job*> jobQueue;
  QHash<quint64, PublicDataModel*> activeSubscriptions;
  bool isConnected;
  QSet<QString> subscriptions;

private slots:
  void handleEvents();
};
