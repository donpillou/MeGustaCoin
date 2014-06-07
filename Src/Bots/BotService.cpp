
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

void BotService::createEntity(quint32 requestId, const void* args, size_t size)
{
  class CreateEntityJob : public Job
  {
  public:
    CreateEntityJob(quint32 requestId, const QByteArray& args) : requestId(requestId), args(args) {}
  private:
    quint32 requestId;
    QByteArray args;
  public: // Event
    virtual bool execute(WorkerThread& workerThread)
    {
      return workerThread.connection.createEntity(requestId, args.constData(), args.size());
    }
  };

  jobQueue.append(new CreateEntityJob(requestId, QByteArray((const char*)args, (int)size)));
  if(thread)
    thread->interrupt();
}

void BotService::updateEntity(quint32 requestId, const void* args, size_t size)
{
  class UpdateEntityJob : public Job
  {
  public:
    UpdateEntityJob(quint32 requestId, const QByteArray& args) : requestId(requestId), args(args) {}
  private:
    quint32 requestId;
    QByteArray args;
  public: // Event
    virtual bool execute(WorkerThread& workerThread)
    {
      return workerThread.connection.updateEntity(requestId, args.constData(), args.size());
    }
  };

  jobQueue.append(new UpdateEntityJob(requestId, QByteArray((const char*)args, (int)size)));
  if(thread)
    thread->interrupt();
}

void BotService::removeEntity(quint32 requestId, BotProtocol::EntityType type, quint32 id)
{
  class RemoveEntityJob : public Job
  {
  public:
    RemoveEntityJob(quint32 requestId, BotProtocol::EntityType type, quint32 id) : requestId(requestId), type(type), id(id) {}
  private:
    quint32 requestId;
    BotProtocol::EntityType type;
    quint32 id;
  public: // Event
    virtual bool execute(WorkerThread& workerThread)
    {
      return workerThread.connection.removeEntity(requestId, type, id);
    }
  };

  jobQueue.append(new RemoveEntityJob(requestId, type, id));
  if(thread)
    thread->interrupt();
}

void BotService::controlEntity(quint32 requestId, const void* args, size_t size)
{
  class ControlEntityJob : public Job
  {
  public:
    ControlEntityJob(quint32 requestId, const QByteArray& args) : requestId(requestId), args(args) {}
  private:
    quint32 requestId;
    QByteArray args;
  public: // Event
    virtual bool execute(WorkerThread& workerThread)
    {
      return workerThread.connection.controlEntity(requestId, args.constData(), args.size());
    }
  };

  jobQueue.append(new ControlEntityJob(requestId, QByteArray((const char*)args, (int)size)));
  if(thread)
    thread->interrupt();
}

void BotService::createMarket(quint32 marketAdapterId, const QString& userName, const QString& key, const QString& secret)
{
  BotProtocol::Market market;
  market.entityType = BotProtocol::market;
  market.entityId = 0;
  market.marketAdapterId = marketAdapterId;
  BotProtocol::setString(market.userName, userName);
  BotProtocol::setString(market.key, key);
  BotProtocol::setString(market.secret, secret);
  createEntity(0, &market, sizeof(market));
}

void BotService::removeMarket(quint32 id)
{
  removeEntity(0, BotProtocol::market, id);
}

void BotService::selectMarket(quint32 id)
{
  BotProtocol::ControlMarket controlMarket;
  controlMarket.entityType = BotProtocol::market;
  controlMarket.entityId = id;
  controlMarket.cmd = BotProtocol::ControlMarket::select;
  controlEntity(0, &controlMarket, sizeof(controlMarket));
}

void BotService::refreshMarketOrders()
{
  EBotService* eBotService = entityManager.getEntity<EBotService>(0);
  BotProtocol::ControlMarket controlMarket;
  controlMarket.entityType = BotProtocol::market;
  controlMarket.entityId = eBotService->getSelectedMarketId();
  controlMarket.cmd = BotProtocol::ControlMarket::refreshOrders;
  controlEntity(0, &controlMarket, sizeof(controlMarket));
}

void BotService::refreshMarketTransactions()
{
  EBotService* eBotService = entityManager.getEntity<EBotService>(0);
  BotProtocol::ControlMarket controlMarket;
  controlMarket.entityType = BotProtocol::market;
  controlMarket.entityId = eBotService->getSelectedMarketId();
  controlMarket.cmd = BotProtocol::ControlMarket::refreshTransactions;
  controlEntity(0, &controlMarket, sizeof(controlMarket));
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
  createEntity(draft.getId(), &order, sizeof(order));
}

void BotService::updateMarketOrder(EBotMarketOrder& order, double price, double amount)
{
  if(order.getState() != EBotMarketOrder::State::open)
    return;
  order.setState(EBotMarketOrder::State::updating);
  entityManager.updatedEntity(order);

  BotProtocol::Order updatedOrder;
  updatedOrder.entityType = BotProtocol::marketOrder;
  updatedOrder.entityId = order.getId();
  updatedOrder.type = (quint8)order.getType();
  updatedOrder.price = price;
  updatedOrder.amount = amount;
  updateEntity(0, &updatedOrder, sizeof(updatedOrder));
}

void BotService::cancelMarketOrder(EBotMarketOrder& order)
{
  if(order.getState() != EBotMarketOrder::State::open)
    return;
  order.setState(EBotMarketOrder::State::canceling);
  entityManager.updatedEntity(order);
  removeEntity(0, BotProtocol::marketOrder, order.getId());
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
  BotProtocol::setString(session.name, name);
  session.botEngineId = engineId;
  session.marketId = marketId;
  session.balanceBase = balanceBase;
  session.balanceComm = balanceComm;
  createEntity(0, &session, sizeof(session));
}

void BotService::removeSession(quint32 id)
{
  removeEntity(0, BotProtocol::session, id);
}

void BotService::startSessionSimulation(quint32 id)
{
  BotProtocol::ControlSession controlSession;
  controlSession.entityType = BotProtocol::session;
  controlSession.entityId = id;
  controlSession.cmd = BotProtocol::ControlSession::startSimulation;
  controlEntity(0, &controlSession, sizeof(controlSession));
}

void BotService::startSession(quint32 id)
{
  BotProtocol::ControlSession controlSession;
  controlSession.entityType = BotProtocol::session;
  controlSession.entityId = id;
  controlSession.cmd = BotProtocol::ControlSession::startLive;
  controlEntity(0, &controlSession, sizeof(controlSession));
}
void BotService::stopSession(quint32 id)
{
  BotProtocol::ControlSession controlSession;
  controlSession.entityType = BotProtocol::session;
  controlSession.entityId = id;
  controlSession.cmd = BotProtocol::ControlSession::stop;
  controlEntity(0, &controlSession, sizeof(controlSession));
}

void BotService::selectSession(quint32 id)
{
  BotProtocol::ControlSession controlSession;
  controlSession.entityType = BotProtocol::session;
  controlSession.entityId = id;
  controlSession.cmd = BotProtocol::ControlSession::select;
  controlEntity(0, &controlSession, sizeof(controlSession));
}

void BotService::addLogMessage(ELogMessage::Type type, const QString& message)
{
  ELogMessage* logMessage = new ELogMessage(type, message);
  entityManager.delegateEntity(*logMessage);
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

void BotService::WorkerThread::addMessage(ELogMessage::Type type, const QString& message)
{
  class LogMessageEvent : public Event
  {
  public:
    LogMessageEvent(ELogMessage::Type type, const QString& message) : type(type), message(message) {}
  private:
    ELogMessage::Type type;
    QString message;
  public: // Event
    virtual void handle(BotService& botService)
    {
      botService.addLogMessage(type, message);
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
        entityManager.removeAll<EBotSessionLogMessage>();
        entityManager.removeAll<EBotSessionMarker>();
        entityManager.removeAll<EBotSessionBalance>();
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

  addMessage(ELogMessage::Type::information, "Connecting to bot service...");

  // create connection
  QStringList addr = server.split(':');
  if(!connection.connect(addr.size() > 0 ? addr[0] : QString(), addr.size() > 1 ? addr[1].toULong() : 0, userName, password))
  {
    addMessage(ELogMessage::Type::error, QString("Could not connect to bot service: %1").arg(connection.getLastError()));
    return;
  }
  addMessage(ELogMessage::Type::information, "Connected to bot service.");
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
  addMessage(ELogMessage::Type::error, QString("Lost connection to bot service: %1").arg(connection.getLastError()));
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
  Entity* entity = createEntity(data, size);
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
  EType eType = getEType((BotProtocol::EntityType)entity.entityType);
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

void BotService::WorkerThread::receivedRemoveAllEntities(const BotProtocol::Entity& entity)
{
  EType eType = getEType((BotProtocol::EntityType)entity.entityType);
  if(eType == EType::none)
    return;

  class RemoveAllEntitiesEvent : public Event
  {
  public:
    RemoveAllEntitiesEvent(EType eType) : eType(eType) {}
  private:
    EType eType;
  public: // Event
    virtual void handle(BotService& botService)
    {
      Entity::Manager& entityManager = botService.entityManager;
      entityManager.removeAll((quint32)eType);
    }
  };
  eventQueue.append(new RemoveAllEntitiesEvent(eType));
  QTimer::singleShot(0, &botService, SLOT(handleEvents()));
}

void BotService::WorkerThread::receivedControlEntityResponse(quint32 requestId, BotProtocol::Entity& entity, size_t size)
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

void BotService::WorkerThread::receivedCreateEntityResponse(quint32 requestId, const BotProtocol::Entity& entity)
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
    CreateEntityResponseEvent(EType eType, quint32 entityId, quint32 requestId) : eType(eType), entityId(entityId), requestId(requestId) {}
  private:
    EType eType;
    quint32 entityId;
    quint32 requestId;
  public: // Event
    virtual void handle(BotService& botService)
    {
      Entity::Manager& entityManager = botService.entityManager;
      switch(eType)
      {
      case EType::botMarketOrder:
        {
          EBotMarketOrderDraft* eBotMarketOrderDraft = entityManager.getEntity<EBotMarketOrderDraft>(requestId);
          if(!eBotMarketOrderDraft)
            return;
          EBotMarketOrder* eBotMarketOrder = new EBotMarketOrder(entityId, *eBotMarketOrderDraft);
          entityManager.delegateEntity(*eBotMarketOrder, *eBotMarketOrderDraft);
        }
        break;
      default:
        break;
      }
    }
  };
  eventQueue.append(new CreateEntityResponseEvent(eType, entity.entityId, requestId));
  QTimer::singleShot(0, &botService, SLOT(handleEvents()));
}

void BotService::WorkerThread::receivedErrorResponse(quint32 requestId, BotProtocol::ErrorResponse& response)
{
  class ErrorResponseEvent : public Event
  {
  public:
    ErrorResponseEvent(quint32 requestId, BotProtocol::EntityType entityType, BotProtocol::MessageType messageType, const QString& errorMessage) : requestId(requestId), entityType(entityType), messageType(messageType), errorMessage(errorMessage) {}
  private:
    quint32 requestId;
    BotProtocol::EntityType entityType;
    BotProtocol::MessageType messageType;
    QString errorMessage;
  public: // Event
    virtual void handle(BotService& botService)
    {
      if(messageType == BotProtocol::createEntity && entityType == BotProtocol::marketOrder)
      {
        Entity::Manager& entityManager = botService.entityManager;
        EBotMarketOrderDraft* eBotMarketOrderDraft = entityManager.getEntity<EBotMarketOrderDraft>(requestId);
        if(eBotMarketOrderDraft)
        {
          eBotMarketOrderDraft->setState(EBotMarketOrder::State::draft);
          entityManager.updatedEntity(*eBotMarketOrderDraft);
        }
      }
      botService.addLogMessage(ELogMessage::Type::error, errorMessage);
    }
  };
  QString errorMessage = BotProtocol::getString(response.errorMessage);
  eventQueue.append(new ErrorResponseEvent(requestId, (BotProtocol::EntityType)response.entityType, (BotProtocol::MessageType)response.messageType, errorMessage));
  QTimer::singleShot(0, &botService, SLOT(handleEvents()));
}

EType BotService::WorkerThread::getEType(BotProtocol::EntityType entityType)
{
  switch(entityType)
  {
  case BotProtocol::session:
    return EType::botSession;
  case BotProtocol::sessionTransaction:
    return EType::botSessionTransaction;
  case BotProtocol::sessionOrder:
    return EType::botSessionOrder;
  case BotProtocol::sessionLogMessage:
    return EType::botSessionLogMessage;
  case BotProtocol::sessionMarker:
    return EType::botSessionMarker;
  case BotProtocol::sessionBalance:
    return EType::botSessionBalance;
  case BotProtocol::market:
    return EType::botMarket;
  case BotProtocol::marketTransaction:
    return EType::botMarketTransaction;
  case BotProtocol::marketOrder:
    return EType::botMarketOrder;
  case BotProtocol::marketBalance:
    return EType::botMarketBalance;
  default:
    return EType::none;
  }
}

Entity* BotService::WorkerThread::createEntity(BotProtocol::Entity& data, size_t size)
{
  switch((BotProtocol::EntityType)data.entityType)
  {
  case BotProtocol::botEngine:
    if(size >= sizeof(BotProtocol::BotEngine))
      return new EBotEngine(*(BotProtocol::BotEngine*)&data);
    break;
  case BotProtocol::session:
    if(size >= sizeof(BotProtocol::Session))
      return new EBotSession(*(BotProtocol::Session*)&data);
    break;
  case BotProtocol::marketAdapter:
    if(size >= sizeof(BotProtocol::MarketAdapter))
      return new EBotMarketAdapter(*(BotProtocol::MarketAdapter*)&data);
    break;
  case BotProtocol::sessionTransaction:
    if(size >= sizeof(BotProtocol::Transaction))
      return new EBotSessionTransaction(*(BotProtocol::Transaction*)&data);
    break;
  case BotProtocol::sessionOrder:
    if(size >= sizeof(BotProtocol::Order))
      return new EBotSessionOrder(*(BotProtocol::Order*)&data);
    break;
  case BotProtocol::sessionMarker:
    if(size >= sizeof(BotProtocol::Marker))
      return new EBotSessionMarker(*(BotProtocol::Marker*)&data);
    break;
  case BotProtocol::sessionLogMessage:
    if(size >= sizeof(BotProtocol::SessionLogMessage))
      return new EBotSessionLogMessage(*(BotProtocol::SessionLogMessage*)&data);
    break;
  case BotProtocol::sessionBalance:
    if(size >= sizeof(BotProtocol::Balance))
      return new EBotSessionBalance(*(BotProtocol::Balance*)&data);
    break;
  case BotProtocol::market:
    if(size >= sizeof(BotProtocol::Market))
      return new EBotMarket(*(BotProtocol::Market*)&data);
    break;
  case BotProtocol::marketTransaction:
    if(size >= sizeof(BotProtocol::Transaction))
      return new EBotMarketTransaction(*(BotProtocol::Transaction*)&data);
    break;
  case BotProtocol::marketOrder:
    if(size >= sizeof(BotProtocol::Order))
      return new EBotMarketOrder(*(BotProtocol::Order*)&data);
    break;
  case BotProtocol::marketBalance:
    if(size >= sizeof(BotProtocol::Balance))
      return new EBotMarketBalance(*(BotProtocol::Balance*)&data);
    break;
  default:
    break;
  }
  return 0;
}
