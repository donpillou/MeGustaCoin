
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

void BotService::createEntity(BotProtocol::EntityType type, const void* args, size_t size)
{
  class CreateEntityJob : public Job
  {
  public:
    CreateEntityJob(BotProtocol::EntityType type, const QByteArray& args) : type(type), args(args) {}
  private:
    BotProtocol::EntityType type;
    QByteArray args;
  public: // Event
    virtual bool execute(WorkerThread& workerThread)
    {
      return workerThread.connection.createEntity(type, args.constData(), args.size());
    }
  };

  jobQueue.append(new CreateEntityJob(type, QByteArray((const char*)args, (int)size)));
  if(thread)
    thread->interrupt();
}

void BotService::removeEntity(BotProtocol::EntityType type, quint32 id)
{
  class RemoveEntityJob : public Job
  {
  public:
    RemoveEntityJob(BotProtocol::EntityType type, quint32 id) : type(type), id(id) {}
  private:
    BotProtocol::EntityType type;
    quint32 id;
  public: // Event
    virtual bool execute(WorkerThread& workerThread)
    {
      return workerThread.connection.removeEntity(type, id);
    }
  };

  jobQueue.append(new RemoveEntityJob(type, id));
  if(thread)
    thread->interrupt();
}

void BotService::controlEntity(BotProtocol::EntityType type, quint32 id, const void* args, size_t size)
{
  class ControlEntityJob : public Job
  {
  public:
    ControlEntityJob(BotProtocol::EntityType type, quint32 id, const QByteArray& args) : type(type), id(id), args(args) {}
  private:
    BotProtocol::EntityType type;
    quint32 id;
    QByteArray args;
  public: // Event
    virtual bool execute(WorkerThread& workerThread)
    {
      return workerThread.connection.controlEntity(type, id, args.constData(), args.size());
    }
  };

  jobQueue.append(new ControlEntityJob(type, id, QByteArray((const char*)args, (int)size)));
  if(thread)
    thread->interrupt();
}


void BotService::createSession(const QString& name, const QString& engine)
{
  BotProtocol::CreateSessionArgs createSession;
  setString(createSession.name, name);
  setString(createSession.engine, engine);
  createEntity(BotProtocol::session, &createSession, sizeof(createSession));
}

void BotService::removeSession(quint32 id)
{
  removeEntity(BotProtocol::session, id);
}

void BotService::startSessionSimulation(quint32 id)
{
  BotProtocol::ControlSessionArgs controlSession;
  controlSession.cmd = BotProtocol::ControlSessionArgs::startSimulation;
  controlEntity(BotProtocol::session, id, &controlSession, sizeof(controlSession));
}

void BotService::stopSession(quint32 id)
{
  BotProtocol::ControlSessionArgs controlSession;
  controlSession.cmd = BotProtocol::ControlSessionArgs::stop;
  controlEntity(BotProtocol::session, id, &controlSession, sizeof(controlSession));
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
  case BotProtocol::session:
    if(size >= sizeof(BotProtocol::Session))
      entity = new EBotSession(header.entityId, *(BotProtocol::Session*)data);
    break;
  case BotProtocol::error:
    if(size >= sizeof(BotProtocol::Error))
    {
      BotProtocol::Error* error = (BotProtocol::Error*)data;
      error->errorMessage[sizeof(error->errorMessage) - 1] = '\0';
      addMessage(LogModel::Type::error, error->errorMessage);
      return;
    }
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
  EType eType = (EType)0;
  switch(header.entityType)
  {
  case BotProtocol::session:
    eType = EType::botSession;
    break;
  }

  class RemoveEntityEvent : public Event
  {
  public:
    RemoveEntityEvent(EType eType, quint32 id) : eType(eType), id(id) {}
  private:
    EType eType;
    quint32 id;
  public: // Event
    virtual void handle(BotService& botService)
    {
        botService.entityManager.removeEntity(eType, id);
    }
  };
  eventQueue.append(new RemoveEntityEvent(eType, header.entityId));
  QTimer::singleShot(0, &botService, SLOT(handleEvents()));
}
