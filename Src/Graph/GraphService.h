
#pragma once

class GraphService : public QObject
{
  Q_OBJECT

public:
  GraphService();
  ~GraphService();

  void start();
  void stop();

  void registerGraphModel(GraphModel& graphModel);
  void unregisterGraphModel(GraphModel& graphModel);

  void enable(GraphModel& graphModel, bool enable);
  void setSize(GraphModel& graphModel, const QSize& size);
  void setMaxAge(GraphModel& graphModel, int maxAge);
  void setEnabledData(GraphModel& graphModel, unsigned int data);

  void addTradeData(GraphModel& graphModel, const QList<EDataTradeData::Trade>& data);
  void addSessionMarker(GraphModel& graphModel, const EUserSessionMarker& marker);
  void clearSessionMarker(GraphModel& graphModel);

private:
  class WorkerThread;

  class Job
  {
  public:
    virtual ~Job() {}
    virtual void execute(WorkerThread& workerThread) = 0;
  };

  class Event
  {
  public:
    virtual ~Event() {}
    virtual void handle(GraphService& graphService) = 0;
  };

  class WorkerThread : public QThread
  {
  public:
    WorkerThread(GraphService& graphService, JobQueue<Event*>& eventQueue, JobQueue<Job*>& jobQueue) :
      graphService(graphService), eventQueue(eventQueue), jobQueue(jobQueue), canceled(false) {}

  private:
    GraphService& graphService;
    JobQueue<Event*>& eventQueue;
    JobQueue<Job*>& jobQueue;
    bool canceled;

  public:
    QHash<GraphModel*, GraphRenderer> graphData;
    QMap<QString, GraphRenderer*> graphDataByName; // sorted

  private: // QThread
    virtual void run();
  };

  struct ChannelData
  {

    QImage image;
    bool updated;

    ChannelData() : updated(false) {}
  };

private:
  WorkerThread* thread;

  JobQueue<Event*> eventQueue;
  JobQueue<Job*> jobQueue;

  QMutex mutex;
  QHash<GraphModel*, ChannelData> channelData;

private slots:
  void handleEvents();
};
