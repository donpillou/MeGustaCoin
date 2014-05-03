
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

void BotService::createEntity(const void* args, size_t size)
{
  class CreateEntityJob : public Job
  {
  public:
    CreateEntityJob(const QByteArray& args) : args(args) {}
  private:
    QByteArray args;
  public: // Event
    virtual bool execute(WorkerThread& workerThread)
    {
      return workerThread.connection.createEntity(args.constData(), args.size());
    }
  };

  jobQueue.append(new CreateEntityJob(QByteArray((const char*)args, (int)size)));
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

void BotService::controlEntity(const void* args, size_t size)
{
  class ControlEntityJob : public Job
  {
  public:
    ControlEntityJob(const QByteArray& args) : args(args) {}
  private:
    QByteArray args;
  public: // Event
    virtual bool execute(WorkerThread& workerThread)
    {
      return workerThread.connection.controlEntity(args.constData(), args.size());
    }
  };

  jobQueue.append(new ControlEntityJob(QByteArray((const char*)args, (int)size)));
  if(thread)
    thread->interrupt();
}

void BotService::createMarket(quint32 marketAdapterId, const QString& userName, const QString& key, const QString& secret)
{
  BotProtocol::Market market;
  market.entityType = BotProtocol::market;
  market.entityId = 0;
  market.marketAdapterId = marketAdapterId;
  setString(market.userName, userName);
  setString(market.key, key);
  setString(market.secret, secret);
  createEntity(&market, sizeof(market));
}

void BotService::removeMarket(quint32 id)
{
  removeEntity(BotProtocol::market, id);
}

void BotService::selectMarket(quint32 id)
{
  BotProtocol::ControlMarket controlMarket;
  controlMarket.entityType = BotProtocol::market;
  controlMarket.entityId = id;
  controlMarket.cmd = BotProtocol::ControlMarket::select;
  controlEntity(&controlMarket, sizeof(controlMarket));
}

void BotService::refreshMarketOrders()
{
  EBotService* eBotService = entityManager.getEntity<EBotService>(0);
  BotProtocol::ControlMarket controlMarket;
  controlMarket.entityType = BotProtocol::market;
  controlMarket.entityId = eBotService->getSelectedMarketId();
  controlMarket.cmd = BotProtocol::ControlMarket::refreshOrders;
  controlEntity(&controlMarket, sizeof(controlMarket));
}

void BotService::refreshMarketTransactions()
{
  EBotService* eBotService = entityManager.getEntity<EBotService>(0);
  BotProtocol::ControlMarket controlMarket;
  controlMarket.entityType = BotProtocol::market;
  controlMarket.entityId = eBotService->getSelectedMarketId();
  controlMarket.cmd = BotProtocol::ControlMarket::refreshTransactions;
  controlEntity(&controlMarket, sizeof(controlMarket));
}

EBotMarketOrderDraft& BotService::createMarketOrderDraft(EBotMarketOrder::Type type, double price)
{
  quint32 id = entityManager.getNewEntityId<EBotMarketOrderDraft>();
  EBotMarketOrderDraft* eBotMarketOrderDraft = new EBotMarketOrderDraft(id, type, QDateTime::currentDateTime(), price);
  entityManager.delegateEntity(*eBotMarketOrderDraft);
  return *eBotMarketOrderDraft;
}

void BotService::submitMarketOrderDraft(EBotMarketOrderDraft& draft)
{
  if(draft.getState() != EBotMarketOrder::State::draft)
    return;
  draft.setState(EBotMarketOrder::State::submitting);
  entityManager.updatedEntity(draft);

  BotProtocol::Order order;
  order.entityType = BotProtocol::marketOrder;
  order.entityId = 0;
  order.type = (quint8)draft.getType();
  order.price = draft.getPrice();
  order.amount = draft.getAmount();
  createEntity(&order, sizeof(order));
}

void BotService::cancelMarketOrder(EBotMarketOrder& order)
{
  if(order.getState() != EBotMarketOrder::State::open)
    return;
  order.setState(EBotMarketOrder::State::canceling);
  entityManager.updatedEntity(order);
  removeEntity(BotProtocol::marketOrder, order.getId());
}

void BotService::removeMarketOrderDraft(EBotMarketOrderDraft& draft)
{
  switch(draft.getState())
  {
  case EBotMarketOrder::State::draft:
  case EBotMarketOrder::State::canceled:
  case EBotMarketOrder::State::closed:
    break;
  default:
    return;
  }
  entityManager.removeEntity<EBotMarketOrderDraft>(draft.getId());
}

void BotService::createSession(const QString& name, quint32 engineId, quint32 marketId, double balanceBase, double balanceComm)
{
  BotProtocol::Session session;
  session.entityType = BotProtocol::session;
  session.entityId = 0;
  setString(session.name, name);
  session.botEngineId = engineId;
  session.marketId = marketId;
  session.balanceBase = balanceBase;
  session.balanceComm = balanceComm;
  createEntity(&session, sizeof(session));
}

void BotService::removeSession(quint32 id)
{
  removeEntity(BotProtocol::session, id);
}

void BotService::startSessionSimulation(quint32 id)
{
  BotProtocol::ControlSession controlSession;
  controlSession.entityType = BotProtocol::session;
  controlSession.entityId = id;
  controlSession.cmd = BotProtocol::ControlSession::startSimulation;
  controlEntity(&controlSession, sizeof(controlSession));
}

void BotService::stopSession(quint32 id)
{
  BotProtocol::ControlSession controlSession;
  controlSession.entityType = BotProtocol::session;
  controlSession.entityId = id;
  controlSession.cmd = BotProtocol::ControlSession::stop;
  controlEntity(&controlSession, sizeof(controlSession));
}

void BotService::selectSession(quint32 id)
{
  BotProtocol::ControlSession controlSession;
  controlSession.entityType = BotProtocol::session;
  controlSession.entityId = id;
  controlSession.cmd = BotProtocol::ControlSession::select;
  controlEntity(&controlSession, sizeof(controlSession));
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
      Entity::Manager& entityManager = botService.entityManager;
      EBotService* eBotService = entityManager.getEntity<EBotService>(0);
      if(state == EBotService::State::connected)
        botService.connected = true;
      else if(state == EBotService::State::offline)
        botService.connected = false;
      eBotService->setState(state);
      if(state == EBotService::State::offline)
      {
        entityManager.removeAll<EBotEngine>();
        entityManager.removeAll<EBotSession>();
        entityManager.removeAll<EBotMarketAdapter>();
        entityManager.removeAll<EBotSessionTransaction>();
        entityManager.removeAll<EBotSessionOrder>();
        entityManager.removeAll<EBotMarketTransaction>();
        entityManager.removeAll<EBotMarketOrder>();
        entityManager.removeAll<EBotMarketOrderDraft>();
        entityManager.removeAll<EBotMarketBalance>();
        entityManager.removeAll<EBotMarket>();
        eBotService->setSelectedMarketId(0);
        eBotService->setSelectedSessionId(0);
      }
      entityManager.updatedEntity(*eBotService);
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

void BotService::WorkerThread::receivedUpdateEntity(BotProtocol::Entity& data, size_t size)
{
  Entity* entity = 0;
  switch((BotProtocol::EntityType)data.entityType)
  {
  case BotProtocol::engine:
    if(size >= sizeof(BotProtocol::BotEngine))
      entity = new EBotEngine(*(BotProtocol::BotEngine*)&data);
    break;
  case BotProtocol::session:
    if(size >= sizeof(BotProtocol::Session))
      entity = new EBotSession(*(BotProtocol::Session*)&data);
    break;
  case BotProtocol::marketAdapter:
    if(size >= sizeof(BotProtocol::MarketAdapter))
      entity = new EBotMarketAdapter(*(BotProtocol::MarketAdapter*)&data);
    break;
  case BotProtocol::sessionTransaction:
    if(size >= sizeof(BotProtocol::Transaction))
      entity = new EBotSessionTransaction(*(BotProtocol::Transaction*)&data);
    break;
  case BotProtocol::sessionOrder:
    if(size >= sizeof(BotProtocol::Order))
      entity = new EBotSessionOrder(*(BotProtocol::Order*)&data);
    break;
  case BotProtocol::market:
    if(size >= sizeof(BotProtocol::Market))
      entity = new EBotMarket(*(BotProtocol::Market*)&data);
    break;
  case BotProtocol::marketTransaction:
    if(size >= sizeof(BotProtocol::Transaction))
      entity = new EBotMarketTransaction(*(BotProtocol::Transaction*)&data);
    break;
  case BotProtocol::marketOrder:
    if(size >= sizeof(BotProtocol::Order))
      entity = new EBotMarketOrder(*(BotProtocol::Order*)&data);
    break;
  case BotProtocol::marketBalance:
    if(size >= sizeof(BotProtocol::MarketBalance))
      entity = new EBotMarketBalance(*(BotProtocol::MarketBalance*)&data);
    break;
  case BotProtocol::error:
    if(size >= sizeof(BotProtocol::Error))
    {
      BotProtocol::Error* error = (BotProtocol::Error*)&data;
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

void BotService::WorkerThread::receivedRemoveEntity(const BotProtocol::Entity& entity)
{
  EType eType = EType::none;
  switch((BotProtocol::EntityType)entity.entityType)
  {
  case BotProtocol::session:
    eType = EType::botSession;
    break;
  case BotProtocol::sessionTransaction:
    eType = EType::botSessionTransaction;
    break;
  case BotProtocol::sessionOrder:
    eType = EType::botSessionOrder;
    break;
  case BotProtocol::market:
    eType = EType::botMarket;
    break;
  case BotProtocol::marketTransaction:
    eType = EType::botMarketTransaction;
    break;
  case BotProtocol::marketOrder:
    eType = EType::botMarketOrder;
    break;
  case BotProtocol::marketBalance:
    eType = EType::botMarketBalance;
    break;
  default:
    break;
  }
  if(eType == EType::none)
    return;

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
      Entity::Manager& entityManager = botService.entityManager;
      switch(eType)
      {
      case EType::botMarketOrder:
        {
          EBotMarketOrder* eBotMarketOrder = entityManager.getEntity<EBotMarketOrder>(id);
          if(!eBotMarketOrder)
            return;
          quint32 draftId = entityManager.getNewEntityId<EBotMarketOrderDraft>();
          EBotMarketOrderDraft* eBotMarketOrderDraft = new EBotMarketOrderDraft(draftId, *eBotMarketOrder);
          entityManager.delegateEntity(*eBotMarketOrderDraft, *eBotMarketOrder);
        }
        break;
      default:
        entityManager.removeEntity(eType, id);
        break;
      }
    }
  };
  eventQueue.append(new RemoveEntityEvent(eType, entity.entityId));
  QTimer::singleShot(0, &botService, SLOT(handleEvents()));
}

void BotService::WorkerThread::receivedControlEntityResponse(BotProtocol::Entity& entity, size_t size)
{
  class ControlEntityResponseEvent : public Event
  {
  public:
    ControlEntityResponseEvent(const QByteArray& response) : response(response) {}
  private:
    QByteArray response;
  public: // Event
    virtual void handle(BotService& botService)
    {
      BotProtocol::Entity* entity = (BotProtocol::Entity*)response.data();
      size_t size = response.size();
      switch((BotProtocol::EntityType)entity->entityType)
      {
      case BotProtocol::market:
        if(size >= sizeof(BotProtocol::ControlMarketResponse))
        {
          BotProtocol::ControlMarketResponse* response = (BotProtocol::ControlMarketResponse*)entity;
          switch((BotProtocol::ControlMarket::Command)response->cmd)
          {
          case BotProtocol::ControlMarket::select:
            {
              Entity::Manager& entityManager = botService.entityManager;
              entityManager.removeAll<EBotMarketOrder>();
              entityManager.removeAll<EBotMarketTransaction>();
              entityManager.removeAll<EBotMarketBalance>();
              EBotService* eBotService = entityManager.getEntity<EBotService>(0);
              eBotService->setSelectedMarketId(entity->entityId);
              entityManager.updatedEntity(*eBotService);
            }
            break;
          default:
            break;
          }
          break;
        }
      case BotProtocol::session:
        if(size >= sizeof(BotProtocol::ControlSessionResponse))
        {
          BotProtocol::ControlSessionResponse* response = (BotProtocol::ControlSessionResponse*)entity;
          switch((BotProtocol::ControlSession::Command)response->cmd)
          {
          case BotProtocol::ControlSession::select:
            {
              Entity::Manager& entityManager = botService.entityManager;
              entityManager.removeAll<EBotSessionOrder>();
              entityManager.removeAll<EBotSessionTransaction>();
              EBotService* eBotService = entityManager.getEntity<EBotService>(0);
              eBotService->setSelectedSessionId(entity->entityId);
              entityManager.updatedEntity(*eBotService);
            }
            break;
          default:
            break;
          }
          break;
        }
      default:
        break;
      }
    }
  };
  eventQueue.append(new ControlEntityResponseEvent(QByteArray((const char*)&entity, (int)size)));
  QTimer::singleShot(0, &botService, SLOT(handleEvents()));
}

void BotService::WorkerThread::receivedCreateEntityResponse(const BotProtocol::CreateResponse& entity)
{
  EType eType = EType::none;
  switch((BotProtocol::EntityType)entity.entityType)
  {
  case BotProtocol::marketOrder:
    eType = EType::botMarketOrder;
    break;
  default:
    return;
  }
  if(eType == EType::none)
    return;

  class CreateEntityResponseEvent : public Event
  {
  public:
    CreateEntityResponseEvent(EType eType, quint32 oldId, quint32 newId, bool success) : eType(eType), oldId(oldId), newId(newId), success(success) {}
  private:
    EType eType;
    quint32 oldId;
    quint32 newId;
    bool success;
  public: // Event
    virtual void handle(BotService& botService)
    {
      Entity::Manager& entityManager = botService.entityManager;
      switch(eType)
      {
      case EType::botMarketOrder:
        if(success)
        {
          EBotMarketOrderDraft* eBotMarketOrderDraft = entityManager.getEntity<EBotMarketOrderDraft>(oldId);
          if(!eBotMarketOrderDraft)
            return;
          EBotMarketOrder* eBotMarketOrder = new EBotMarketOrder(newId, *eBotMarketOrderDraft);
          entityManager.delegateEntity(*eBotMarketOrder, *eBotMarketOrderDraft);
        }
        else
        {
          EBotMarketOrderDraft* eBotMarketOrderDraft = entityManager.getEntity<EBotMarketOrderDraft>(oldId);
          if(!eBotMarketOrderDraft)
            return;
          eBotMarketOrderDraft->setState(EBotMarketOrder::State::draft);
          entityManager.updatedEntity(*eBotMarketOrderDraft);
        }
        break;
      default:
        break;
      }
    }
  };
  eventQueue.append(new CreateEntityResponseEvent(eType, entity.entityId, entity.id, entity.success != 0));
  QTimer::singleShot(0, &botService, SLOT(handleEvents()));
}
