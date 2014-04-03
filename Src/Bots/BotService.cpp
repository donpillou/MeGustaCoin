
#include "stdafx.h"

BotService::BotService(DataModel& dataModel, Entity::Manager& entityManager) :
  dataModel(dataModel), entityManager(entityManager), thread(0), connected(false) {}

BotService::~BotService()
{
  stop();
}

void BotService::start(const QString& server, const QString& userName, const QString& password)
{
  if(thread)
  {
     if(server == thread->getServer() && userName == thread->getUserName() && password == thread->getPassword())
       return;
     stop();
     Q_ASSERT(!thread);
  }

  thread = new WorkerThread(*this, eventQueue, jobQueue, server, userName, password);
  thread->start();
}

void BotService::stop()
{
  if(!thread)
    return;

  jobQueue.append(0); // cancel worker thread
  thread->interrupt();
  thread->wait();
  delete thread;
  thread = 0;

  handleEvents(); // better than qDeleteAll(eventQueue.getAll()); ;)
  qDeleteAll(jobQueue.getAll());
}

void BotService::createSession(const QString& name, const QString& engine)
{
  class CreateSessionJob : public Job
  {
  public:
    CreateSessionJob(const QString& name, const QString& engine) : name(name), engine(engine) {}
  private:
    QString name;
    QString engine;
  public: // Event
    virtual bool execute(WorkerThread& workerThread)
    {
      return workerThread.connection.createSession(name, engine);
    }
  };

  jobQueue.append(new CreateSessionJob(name, engine));
  if(thread)
    thread->interrupt();
}

void BotService::handleEvents()
{
  for(;;)
  {
    Event* event = 0;
    if(!eventQueue.get(event, 0) || !event)
      break;
    event->handle(*this);
    delete event;
  }
}

void BotService::WorkerThread::interrupt()
{
  connection.interrupt();
}

void BotService::WorkerThread::addMessage(LogModel::Type type, const QString& message)
{
  class LogMessageEvent : public Event
  {
  public:
    LogMessageEvent(LogModel::Type type, const QString& message) : type(type), message(message) {}
  private:
    LogModel::Type type;
    QString message;
  public: // Event
    virtual void handle(BotService& botService)
    {
        botService.dataModel.logModel.addMessage(type, message);
    }
  };
  eventQueue.append(new LogMessageEvent(type, message));
  QTimer::singleShot(0, &botService, SLOT(handleEvents()));
}

void BotService::WorkerThread::setState(EBotService::State state)
{
  class SetStateEvent : public Event
  {
  public:
    SetStateEvent(EBotService::State state) : state(state) {}
  private:
    EBotService::State state;
  private: // Event
    virtual void handle(BotService& botService)
    {
      if(state == EBotService::State::connected)
        botService.connected = true;
      else if(state == EBotService::State::offline)
        botService.connected = false;
      EBotService* eBotService = botService.entityManager.getEntity<EBotService>(0);
      eBotService->setState(state);
      //botService.dataModel.botsModel.setState(state);
    }
  };
  eventQueue.append(new SetStateEvent(state));
  QTimer::singleShot(0, &botService, SLOT(handleEvents()));
}

void BotService::WorkerThread::process()
{
  Job* job;
  while(jobQueue.get(job, 0))
  {
    if(!job)
    {
      canceled = true;
      return;
    }
    delete job;
  }

  addMessage(LogModel::Type::information, "Connecting to bot service...");

  // create connection
  QStringList addr = server.split(':');
  if(!connection.connect(addr.size() > 0 ? addr[0] : QString(), addr.size() > 1 ? addr[1].toULong() : 0, userName, password))
  {
    addMessage(LogModel::Type::error, QString("Could not connect to bot service: %1").arg(connection.getLastError()));
    return;
  }
  addMessage(LogModel::Type::information, "Connected to bot service.");
  setState(EBotService::State::connected);

  // loop
  for(;;)
  {
    while(jobQueue.get(job, 0))
    {
      if(!job)
      {
        canceled = true;
        return;
      }
      if(!job->execute(*this))
        goto error;
      delete job;
    }

    if(!connection.process(*this))
      break;
  }

error:
  addMessage(LogModel::Type::error, QString("Lost connection to bot service: %1").arg(connection.getLastError()));
}

void BotService::WorkerThread::run()
{
  while(!canceled)
  {
    setState(EBotService::State::connecting);
    process();
    setState(EBotService::State::offline);
    if(canceled)
      return;
    sleep(10);
  }
}

void BotService::WorkerThread::receivedUpdateEntity(const BotProtocol::Header& header, char* data, size_t size)
{
  Entity* entity = 0;
  switch((BotProtocol::EntityType)header.entityType)
  {
  case BotProtocol::engine:
    if(size >= sizeof(BotProtocol::Engine))
      entity = new EBotEngine(header.entityId, *(BotProtocol::Engine*)data);
    break;
  default:
    return;
  }
  if(!entity)
    return;

  class UpdateEntityEvent : public Event
  {
  public:
    UpdateEntityEvent(Entity& entity) : entity(entity) {}
  private:
    Entity& entity;
  public: // Event
    virtual void handle(BotService& botService)
    {
        botService.entityManager.delegateEntity(entity);
    }
  };
  eventQueue.append(new UpdateEntityEvent(*entity));
  QTimer::singleShot(0, &botService, SLOT(handleEvents()));
}

void BotService::WorkerThread::receivedRemoveEntity(const BotProtocol::Header& header)
{
}

//void BotService::WorkerThread::receivedErrorResponse(const QString& errorMessage)
//{
//  addMessage(LogModel::Type::error, errorMessage);
//}

//void BotService::WorkerThread::receivedEngine(const QString& engine)
//{
//  class EngineMessageEvent : public Event
//  {
//  public:
//    EngineMessageEvent(const QString& engine) : engine(engine) {}
//  private:
//    QString engine;
//  public: // Event
//    virtual void handle(BotService& botService)
//    {
//        botService.dataModel.botsModel.addEngine(engine);
//    }
//  };
//  eventQueue.append(new EngineMessageEvent(engine));
//  QTimer::singleShot(0, &botService, SLOT(handleEvents()));
//}

//void BotService::WorkerThread::receivedSession(quint32 id, const QString& name, const QString& engine)
//{
//  class SessionMessageEvent : public Event
//  {
//  public:
//    SessionMessageEvent(quint32 id, const QString& name, const QString& engine) : id(id), name(name), engine(engine) {}
//  private:
//    quint32 id;
//    QString name;
//    QString engine;
//  public: // Event
//    virtual void handle(BotService& botService)
//    {
//        botService.dataModel.botsModel.addSession(id, name, engine);
//    }
//  };
//  eventQueue.append(new SessionMessageEvent(id, name, engine));
//  QTimer::singleShot(0, &botService, SLOT(handleEvents()));
//}
