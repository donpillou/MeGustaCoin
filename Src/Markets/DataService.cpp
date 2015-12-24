
#include "stdafx.h"

DataService::DataService(Entity::Manager& globalEntityManager) :
  globalEntityManager(globalEntityManager), thread(0), isConnected(false) {}

DataService::~DataService()
{
  stop();
}

void DataService::start(const QString& server, const QString& userName, const QString& password)
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

void DataService::stop()
{
  if(!thread)
    return;

  jobQueue.append(0, 0); // cancel worker thread
  thread->interrupt();
  thread->wait();
  delete thread;
  thread = 0;

  handleEvents();
  Q_ASSERT(eventQueue.isEmpty());
  qDeleteAll(jobQueue.getAll());
}

void DataService::subscribe(const QString& channelName, Entity::Manager& channelEntityManager)
{
  if(subscriptions.contains(channelName))
    return;
  subscriptions.insert(channelName, &channelEntityManager);
  if(!isConnected)
    return;

  class SubscriptionJob : public Job, public Event
  {
  public:
    SubscriptionJob(const QString& channelName, quint32 channelId, quint64 lastReceivedTradeId) : channelName(channelName), channelId(channelId), lastReceivedTradeId(lastReceivedTradeId), eTradeData(0), eTickerData(0) {}
    ~SubscriptionJob() {delete eTradeData;}
  private:
    QString channelName;
    quint32 channelId;
    quint64 lastReceivedTradeId;
    EDataTradeData* eTradeData;
    EDataTickerData* eTickerData;
  private: // Job
    virtual bool execute(WorkerThread& workerThread)
    {
      WorkerThread::SubscriptionData& data = workerThread.subscriptionData[channelId];
      data.channelName = channelName;
      delete data.eTradeData;
      data.eTradeData = new EDataTradeData;
      delete data.eTickerData;
      data.eTickerData = 0;

      if(!workerThread.connection.subscribeMarket(channelId, lastReceivedTradeId))
        return workerThread.addMessage(ELogMessage::Type::error, QString("Could not subscribe to channel %1: %2").arg(channelName, workerThread.connection.getLastError())), false;
      
      eTradeData = data.eTradeData;
      data.eTradeData = 0;
      eTickerData = data.eTickerData;
      data.eTickerData = 0;
      return true;
    }
  private: // Event
    virtual void handle(DataService& dataService)
    {
      Entity::Manager* channelEntityManager = dataService.getSubscription(channelName);
      if(channelEntityManager)
      {
        channelEntityManager->delegateEntity(*eTradeData);
        eTradeData = 0;
        if(eTickerData)
        {
          channelEntityManager->delegateEntity(*eTickerData);
          eTickerData = 0;
        }
        dataService.activeSubscriptions[channelId] = channelEntityManager;
        EDataSubscription* eDataSubscription = channelEntityManager->getEntity<EDataSubscription>(0);
        eDataSubscription->setState(EDataSubscription::State::subscribed);
        channelEntityManager->updatedEntity(*eDataSubscription);
        EDataMarket* eDataMarket = dataService.globalEntityManager.getEntity<EDataMarket>(channelId);
        dataService.addLogMessage(ELogMessage::Type::information, QString("Subscribed to channel %1.").arg(eDataMarket->getName()));
      }
    }
  };

  addLogMessage(ELogMessage::Type::information, QString("Subscribing to channel %1...").arg(channelName));

  quint32 channelId = getChannelId(channelName);
  if(channelId)
  {
    EDataTradeData* eDataTradeData = channelEntityManager.getEntity<EDataTradeData>(0);
    quint64 lastReceivedTradeId = 0;
    if(eDataTradeData)
    {
      const QList<EDataTradeData::Trade>& data = eDataTradeData->getData();
      if(!data.isEmpty())
        lastReceivedTradeId = data.back().id;
    }
    bool wasEmpty;
    jobQueue.append(new SubscriptionJob(channelName, channelId, lastReceivedTradeId), &wasEmpty);
    if(wasEmpty && thread)
      thread->interrupt();
  }
  EDataSubscription* eDataSubscription = channelEntityManager.getEntity<EDataSubscription>(0);
  eDataSubscription->setState(EDataSubscription::State::subscribing);
  channelEntityManager.updatedEntity(*eDataSubscription);
}

void DataService::unsubscribe(const QString& channelName)
{
  QHash<QString, Entity::Manager*>::Iterator it = subscriptions.find(channelName);
  if(it == subscriptions.end())
    return;
  subscriptions.erase(it);
  if(!isConnected)
    return;

  class UnsubscriptionJob : public Job, public Event
  {
  public:
    UnsubscriptionJob(const QString& channelName, quint32 channelId) : channelName(channelName), channelId(channelId) {}
  private:
    QString channelName;
    quint32 channelId;
  private: // Job
    virtual bool execute(WorkerThread& workerThread)
    {
      if(!workerThread.connection.unsubscribeMarket(channelId))
        return workerThread.addMessage(ELogMessage::Type::error, QString("Could not unsubscribe from channel %1: %2").arg(channelName, workerThread.connection.getLastError())), false;
      return true;
    }
  private: // Event
    virtual void handle(DataService& dataService)
    {
      QHash<quint32, Entity::Manager*>::Iterator it = dataService.activeSubscriptions.find(channelId);
      if(it == dataService.activeSubscriptions.end())
        return;
      Entity::Manager* channelEntityManager = it.value();
      dataService.activeSubscriptions.erase(it);
      EDataSubscription* eDataSubscription = channelEntityManager->getEntity<EDataSubscription>(0);
      eDataSubscription->setState(EDataSubscription::State::unsubscribed);
      dataService.addLogMessage(ELogMessage::Type::information, QString("Unsubscribed from channel %1.").arg(channelName));
    }
  };

  quint32 channelId = getChannelId(channelName);
  addLogMessage(ELogMessage::Type::information, QString("Unsubscribing from channel %1...").arg(channelName));
  bool wasEmpty;
  jobQueue.append(new UnsubscriptionJob(channelName, channelId), &wasEmpty);
  if(wasEmpty && thread)
    thread->interrupt();
}

Entity::Manager* DataService::getSubscription(const QString& channel)
{
  QHash<QString, Entity::Manager*>::Iterator it = subscriptions.find(channel);
  if(it == subscriptions.end())
    return 0;
  return it.value();
}

void DataService::handleEvents()
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

void DataService::addLogMessage(ELogMessage::Type type, const QString& message)
{
  ELogMessage* logMessage = new ELogMessage(type, message);
  globalEntityManager.delegateEntity(*logMessage);
}

quint32 DataService::getChannelId(const QString& channelName)
{
  QList<EDataMarket*> eDataMarkets;
  globalEntityManager.getAllEntities<EDataMarket>(eDataMarkets);
  for(QList<EDataMarket*>::Iterator i = eDataMarkets.begin(), end = eDataMarkets.end(); i != end; ++i)
    if((*i)->getName() == channelName)
      return (*i)->getId();
  return 0;
}

void DataService::WorkerThread::interrupt()
{
  connection.interrupt();
}

void DataService::WorkerThread::addMessage(ELogMessage::Type type, const QString& message)
{
  ELogMessage* logMessage = new ELogMessage(type, message);
  delegateEntity(logMessage);
}

void DataService::WorkerThread::delegateEntity(Entity* entity)
{
  class DelegateEntityEvent : public Event
  {
  public:
    DelegateEntityEvent(Entity* entity) : entity(entity) {}
    ~DelegateEntityEvent() {delete entity;}
  private:
    Entity* entity;
  public: // Event
    virtual void handle(DataService& dataService)
    {
      dataService.globalEntityManager.delegateEntity(*entity);
      entity = 0;
    }
  };

  bool wasEmpty;
  eventQueue.append(new DelegateEntityEvent(entity), &wasEmpty);
  if(wasEmpty)
    QTimer::singleShot(0, &dataService, SLOT(handleEvents()));
}

void DataService::WorkerThread::removeEntity(EType type, quint64 id)
{
  class RemoveEntityEvent : public Event
  {
  public:
    RemoveEntityEvent(EType type, quint64 id) : type(type), id(id) {}
  private:
    EType type;
    quint64 id;
  public: // Event
    virtual void handle(DataService& dataService)
    {
      switch(type)
      {
      case EType::userBroker:
        {
          EDataService* eDataService = dataService.globalEntityManager.getEntity<EDataService>(0);
          if(eDataService->getSelectedBrokerId() == id)
          {
            eDataService->setSelectedBrokerId(0);
            dataService.globalEntityManager.updatedEntity(*eDataService);
          }
        }
        break;
      case EType::botSession:
        {
          EDataService* eDataService = dataService.globalEntityManager.getEntity<EDataService>(0);
          if(eDataService->getSelectedSessionId() == id)
          {
            eDataService->setSelectedSessionId(0);
            dataService.globalEntityManager.updatedEntity(*eDataService);
          }
        }
        break;
        // todo: handle other entity types
      default:
        Q_ASSERT(false);
        break;
      }
      dataService.globalEntityManager.removeEntity(type, id);
    }
  };

  bool wasEmpty;
  eventQueue.append(new RemoveEntityEvent(type, id), &wasEmpty);
  if(wasEmpty)
    QTimer::singleShot(0, &dataService, SLOT(handleEvents()));
}

void DataService::WorkerThread::setState(EDataService::State state)
{
  class SetStateEvent : public Event
  {
  public:
    SetStateEvent(EDataService::State state) : state(state) {}
  private:
    EDataService::State state;
  private: // Event
    virtual void handle(DataService& dataService)
    {
      Entity::Manager& globalEntityManager = dataService.globalEntityManager;
      EDataService* eDataService = globalEntityManager.getEntity<EDataService>(0);
      if(state == EDataService::State::connected)
      {
        dataService.isConnected = true;
        QHash<QString, Entity::Manager*> subscriptions;
        subscriptions.swap(dataService.subscriptions);
        for(QHash<QString, Entity::Manager*>::Iterator i = subscriptions.begin(), end = subscriptions.end(); i != end; ++i)
          dataService.subscribe(i.key(), *i.value());
      }
      else if(state == EDataService::State::offline)
      {
        dataService.isConnected = false;
        for(QHash<quint32, Entity::Manager*>::ConstIterator i = dataService.activeSubscriptions.begin(), end = dataService.activeSubscriptions.end(); i != end; ++i)
        {
          Entity::Manager* channelEntityManager = i.value();
          EDataSubscription* eDataSubscription = channelEntityManager->getEntity<EDataSubscription>(0);
          if(eDataSubscription)
            eDataSubscription->setState(EDataSubscription::State::unsubscribed);
        }
        dataService.activeSubscriptions.clear();
        globalEntityManager.removeAll<EBotType>();
        globalEntityManager.removeAll<EBotSession>();
        globalEntityManager.removeAll<EBrokerType>();
        globalEntityManager.removeAll<EBotSessionTransaction>();
        globalEntityManager.removeAll<EBotSessionItem>();
        globalEntityManager.removeAll<EBotSessionProperty>();
        globalEntityManager.removeAll<EBotSessionItemDraft>();
        globalEntityManager.removeAll<EBotSessionOrder>();
        globalEntityManager.removeAll<EBotSessionLogMessage>();
        globalEntityManager.removeAll<EBotSessionMarker>();
        globalEntityManager.removeAll<EBotMarketTransaction>();
        globalEntityManager.removeAll<EBotMarketOrder>();
        globalEntityManager.removeAll<EBotMarketOrderDraft>();
        globalEntityManager.removeAll<EUserBrokerBalance>();
        globalEntityManager.removeAll<EUserBroker>();
        globalEntityManager.removeAll<EProcess>();
        eDataService->setSelectedBrokerId(0);
        eDataService->setSelectedSessionId(0);
        eDataService->setLoadingBrokerOrders(false);
        eDataService->setLoadingBrokerTransactions(false);
      }
      eDataService->setState(state);
      if(state == EDataService::State::offline)
        globalEntityManager.removeAll<EDataMarket>();
      globalEntityManager.updatedEntity(*eDataService);
    }
  };

  bool wasEmpty;
  eventQueue.append(new SetStateEvent(state), &wasEmpty);
  if(wasEmpty)
    QTimer::singleShot(0, &dataService, SLOT(handleEvents()));
}

void DataService::WorkerThread::process()
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

  addMessage(ELogMessage::Type::information, "Connecting to data service...");

  // create connection
  QStringList addr = server.split(':');
  if(!connection.connect(addr.size() > 0 ? addr[0] : QString(), addr.size() > 1 ? addr[1].toULong() : 0, userName, password, *this))
    return addMessage(ELogMessage::Type::error, QString("Could not connect to data service: %1").arg(connection.getLastError()));
  addMessage(ELogMessage::Type::information, "Connected to data service.");

  setState(EDataService::State::connected);

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
      {
        delete job;
        if(!connection.isConnected())
          break;
      }
      else
      {
        Event* event = dynamic_cast<Event*>(job);
        if(event)
        {
          bool wasEmpty;
          eventQueue.append(event, &wasEmpty);
          if(wasEmpty)
            QTimer::singleShot(0, &dataService, SLOT(handleEvents()));
        }
        else
          delete job;
      }
    }
    if(!connection.isConnected())
      break;

    if(!connection.process())
    {
      addMessage(ELogMessage::Type::error, QString("Lost connection to data service: %1").arg(connection.getLastError()));
      break;
    }
  }
}

void DataService::WorkerThread::run()
{
  while(!canceled)
  {
    setState(EDataService::State::connecting);
    process();
    setState(EDataService::State::offline);

    subscriptionData.clear();

    if(canceled)
      return;
    sleep(10);
  }
}

void DataService::WorkerThread::receivedMarket(quint32 tableId, const QString& channelName)
{
  delegateEntity(new EDataMarket(tableId, channelName));
}

void DataService::WorkerThread::receivedBroker(quint32 brokerId,const meguco_user_broker_entity& broker, const QString& userName)
{
  delegateEntity(new EUserBroker(brokerId, broker, userName));
}

void DataService::WorkerThread::removedBroker(quint32 brokerId)
{
  removeEntity(EType::userBroker, brokerId);
}

void DataService::WorkerThread::receivedSession(quint32 sessionId, const QString& name, const meguco_user_session_entity& session)
{
  delegateEntity(new EBotSession(sessionId, name, session));
}

void DataService::WorkerThread::removedSession(quint32 sesionId)
{
  removeEntity(EType::botSession, sesionId);
}

void DataService::WorkerThread::receivedMarketTrade(quint32 marketId, const meguco_trade_entity& tradeData, qint64 timeOffset)
{
  EDataTradeData::Trade trade;
  trade.id = tradeData.entity.id;
  trade.time = tradeData.entity.time + timeOffset;
  trade.price = tradeData.price;
  trade.amount = tradeData.amount;

  SubscriptionData& data = subscriptionData[marketId];
  if(data.eTradeData)
    data.eTradeData->addTrade(trade);
  else // live trade
  {
    class AddTradeEvent : public Event
    {
    public:
      AddTradeEvent(quint32 marketId, const EDataTradeData::Trade& trade) : marketId(marketId), trade(trade) {}
    private:
      quint32 marketId;
      EDataTradeData::Trade trade;
    private: // Event
      virtual void handle(DataService& dataService)
      {
        QHash<quint32, Entity::Manager*>::ConstIterator it = dataService.activeSubscriptions.find(marketId);
        if(it == dataService.activeSubscriptions.end())
          return;
        Entity::Manager* channelEntityManager = it.value();

        EDataTradeData* eDataTradeData = channelEntityManager->getEntity<EDataTradeData>(0);
        if(eDataTradeData)
        {
          eDataTradeData->setData(trade);
          channelEntityManager->updatedEntity(*eDataTradeData);
        }
        else
        {
          eDataTradeData = new EDataTradeData(trade);
          channelEntityManager->delegateEntity(*eDataTradeData);
        }
      }
    };

    bool wasEmpty;
    eventQueue.append(new AddTradeEvent(marketId, trade), &wasEmpty);
    if(wasEmpty)
      QTimer::singleShot(0, &dataService, SLOT(handleEvents()));
  }
}

void DataService::WorkerThread::receivedMarketTicker(quint32 marketId, const meguco_ticker_entity& ticker)
{
  SubscriptionData& data = subscriptionData[marketId];
  if(data.eTradeData)
  {
    delete data.eTickerData;
    data.eTickerData = new EDataTickerData(ticker.ask, ticker.bid);
  }
  else
  {
    class AddTickerEvent : public Event
    {
    public:
      AddTickerEvent(quint64 marketId, const meguco_ticker_entity& ticker) : marketId(marketId), ticker(ticker) {}
    private:
      quint64 marketId;
      meguco_ticker_entity ticker;
    private: // Event
      virtual void handle(DataService& dataService)
      {
        QHash<quint32, Entity::Manager*>::ConstIterator it = dataService.activeSubscriptions.find(marketId);
        if(it == dataService.activeSubscriptions.end())
          return;
        Entity::Manager* channelEntityManager = it.value();
        EDataTickerData* eDataTickerData = channelEntityManager->getEntity<EDataTickerData>(0);
        if(eDataTickerData)
        {
          eDataTickerData->setData(ticker.ask, ticker.bid);
          channelEntityManager->updatedEntity(*eDataTickerData);
        }
        else
        {
          eDataTickerData = new EDataTickerData(ticker.ask, ticker.bid);
          channelEntityManager->delegateEntity(*eDataTickerData);
        }
      }
    };

    bool wasEmpty;
    eventQueue.append(new AddTickerEvent(marketId, ticker), &wasEmpty);
    if(wasEmpty)
      QTimer::singleShot(0, &dataService, SLOT(handleEvents()));
  }
}

void DataService::WorkerThread::receivedBrokerType(const meguco_broker_type_entity& brokerType, const QString& name)
{
  delegateEntity(new EBrokerType(brokerType, name));
}

void DataService::WorkerThread::receivedBotType(const meguco_bot_type_entity& botType, const QString& name)
{
  delegateEntity(new EBotType(botType, name));
}

void  DataService::WorkerThread::receivedBrokerBalance(const meguco_user_broker_balance_entity& balance)
{
  delegateEntity(new EUserBrokerBalance(balance));
}

void DataService::WorkerThread::removedBrokerBalance(quint64 balanceId)
{
  removeEntity(EType::userBrokerBalance, balanceId);
}

void DataService::WorkerThread::receivedBrokerOrder(const meguco_user_broker_order_entity& order)
{
  delegateEntity(new EBotMarketOrder(order));
}

void DataService::WorkerThread::removedBrokerOrder(quint64 orderId)
{
  removeEntity(EType::botMarketOrder, orderId);
}

void DataService::WorkerThread::receivedBrokerTransaction(const meguco_user_broker_transaction_entity& transaction)
{
  delegateEntity(new EBotMarketTransaction(transaction));
}

void DataService::WorkerThread::removedBrokerTransaction(quint64 transactionId)
{
  removeEntity(EType::botMarketTransaction, transactionId);
}

void DataService::WorkerThread::receivedBrokerLog(const meguco_log_entity& log, const QString& message)
{
  delegateEntity(new ELogMessage((ELogMessage::Type)log.type, message));
}

void DataService::WorkerThread::receivedSessionOrder(const meguco_user_broker_order_entity& order)
{
  delegateEntity(new EBotSessionOrder(order));
}

void DataService::WorkerThread::removedSessionOrder(quint64 orderId)
{
  removeEntity(EType::botSessionOrder, orderId);
}

void DataService::WorkerThread::receivedSessionTransaction(const meguco_user_broker_transaction_entity& transaction)
{
  delegateEntity(new EBotSessionTransaction(transaction));
}

void DataService::WorkerThread::removedSessionTransaction(quint64 transactionId)
{
  removeEntity(EType::botSessionTransaction, transactionId);
}

void DataService::WorkerThread::receivedSessionAsset(const meguco_user_session_asset_entity& asset)
{
  delegateEntity(new EBotSessionItem(asset));
}

void DataService::WorkerThread::removedSessionAsset(quint64 assertId)
{
  removeEntity(EType::botSessionItem, assertId);
}

void DataService::WorkerThread::receivedSessionLog(const meguco_log_entity& log, const QString& message)
{
  delegateEntity(new EBotSessionLogMessage(log, message));
}

void DataService::WorkerThread::removedSessionLog(quint64 logId)
{
  removeEntity(EType::botSessionLogMessage, logId);
}

void DataService::WorkerThread::receivedSessionProperty(const meguco_user_session_property_entity& property, const QString& name, const QString& value, const QString& unit)
{
  delegateEntity(new EBotSessionProperty(property, name, value, unit));
}

void DataService::WorkerThread::removedSessionProperty(quint64 propertyId)
{
  removeEntity(EType::botSessionProperty, propertyId);
}

void DataService::WorkerThread::receivedProcess(const meguco_process_entity& process, const QString& cmd)
{
  delegateEntity(new EProcess(process, cmd));
}

void DataService::WorkerThread::removedProcess(quint64 processId)
{
  removeEntity(EType::process, processId);
}

void DataService::createBroker(quint64 marketId, const QString& userName, const QString& key, const QString& secret)
{
  class CreateBrokerJob : public Job
  {
  public:
    CreateBrokerJob(quint64 marketId, const QString& userName, const QString& key, const QString& secret) : marketId(marketId), userName(userName), key(key), secret(secret) {}
  private:
    quint64 marketId;
    QString userName;
    QString key;
    QString secret;
  private: // Job
    virtual bool execute(WorkerThread& workerThread)
    {
      if(!workerThread.connection.createBroker(marketId, userName, key, secret))
        return workerThread.addMessage(ELogMessage::Type::error, QString("Could not create broker: %1").arg(workerThread.connection.getLastError())), false;
      return true;
    }
  };

  bool wasEmpty;
  jobQueue.append(new CreateBrokerJob(marketId, userName, key, secret), &wasEmpty);
  if(wasEmpty && thread)
    thread->interrupt();
}

void DataService::removeBroker(quint32 brokerId)
{
  class RemoveBrokerJob : public Job
  {
  public:
    RemoveBrokerJob(quint32 brokerId) : brokerId(brokerId) {}
  private:
    quint32 brokerId;
  private: // Job
    virtual bool execute(WorkerThread& workerThread)
    {
      if(!workerThread.connection.removeBroker(brokerId))
        return workerThread.addMessage(ELogMessage::Type::error, QString("Could not remove broker: %1").arg(workerThread.connection.getLastError())), false;
      return true;
    }
  };

  bool wasEmpty;
  jobQueue.append(new RemoveBrokerJob(brokerId), &wasEmpty);
  if(wasEmpty && thread)
    thread->interrupt();
}

void DataService::selectBroker(quint32 brokerId)
{
  class SelectBrokerJob : public Job, public Event
  {
  public:
    SelectBrokerJob(quint32 brokerId) : brokerId(brokerId) {}
  private:
    quint32 brokerId;
  private: // Job
    virtual bool execute(WorkerThread& workerThread)
    {
      if(!workerThread.connection.selectBroker(brokerId))
        return workerThread.addMessage(ELogMessage::Type::error, QString("Could not select broker: %1").arg(workerThread.connection.getLastError())), false;
      return true;
    }
  private: // Event
    virtual void handle(DataService& dataService)
    {
      Entity::Manager& globalEntityManager = dataService.globalEntityManager;
      EDataService* eDataService = globalEntityManager.getEntity<EDataService>(0);
      eDataService->setSelectedBrokerId(brokerId);
      globalEntityManager.updatedEntity(*eDataService);
    }
  };

  bool wasEmpty;
  jobQueue.append(new SelectBrokerJob(brokerId), &wasEmpty);
  if(wasEmpty && thread)
    thread->interrupt();
}

void DataService::refreshBrokerOrders()
{
  EDataService* eDataService = globalEntityManager.getEntity<EDataService>(0);
  eDataService->setLoadingBrokerOrders(true);
  globalEntityManager.updatedEntity(*eDataService);

  class RefreshBrokerOrdersJob : public Job, public Event
  {
  private: // Job
    virtual bool execute(WorkerThread& workerThread)
    {
      bool controlResult;
      if(!workerThread.connection.controlBroker(meguco_user_broker_control_refresh_orders, controlResult))
          return workerThread.addMessage(ELogMessage::Type::error, QString("Could not refresh broker orders: %1").arg(workerThread.connection.getLastError())), false;
      return true;
    }
  private: // Event
    virtual void handle(DataService& dataService)
    {
      // unset loading state
      EDataService* eDataService = dataService.globalEntityManager.getEntity<EDataService>(0);
      eDataService->setLoadingBrokerOrders(false);
      dataService.globalEntityManager.updatedEntity(*eDataService);
    }
  };

  bool wasEmpty;
  jobQueue.append(new RefreshBrokerOrdersJob(), &wasEmpty);
  if(wasEmpty && thread)
    thread->interrupt();
}

void DataService::refreshBrokerTransactions()
{
  EDataService* eDataService = globalEntityManager.getEntity<EDataService>(0);
  eDataService->setLoadingBrokerTransactions(true);
  globalEntityManager.updatedEntity(*eDataService);

  class RefreshBrokerTransactionsJob : public Job, public Event
  {
  private: // Job
    virtual bool execute(WorkerThread& workerThread)
    {
      bool controlResult;
      if(!workerThread.connection.controlBroker(meguco_user_broker_control_refresh_transactions, controlResult))
          return workerThread.addMessage(ELogMessage::Type::error, QString("Could not refresh broker transactions: %1").arg(workerThread.connection.getLastError())), false;
      return true;
    }
  private: // Event
    virtual void handle(DataService& dataService)
    {
      // unset loading state
      EDataService* eDataService = dataService.globalEntityManager.getEntity<EDataService>(0);
      eDataService->setLoadingBrokerTransactions(false);
      dataService.globalEntityManager.updatedEntity(*eDataService);

    }
  };

  bool wasEmpty;
  jobQueue.append(new RefreshBrokerTransactionsJob(), &wasEmpty);
  if (wasEmpty && thread)
    thread->interrupt();
}

void DataService::refreshBrokerBalance()
{
  class RefreshBrokerBalanceJob : public Job
  {
  private: // Job
    virtual bool execute(WorkerThread& workerThread)
    {
      bool controlResult;
      if(!workerThread.connection.controlBroker(meguco_user_broker_control_refresh_balance, controlResult))
        return workerThread.addMessage(ELogMessage::Type::error, QString("Could not refresh broker balance: %1").arg(workerThread.connection.getLastError())), false;
      return true;
    }
  };

  bool wasEmpty;
  jobQueue.append(new RefreshBrokerBalanceJob(), &wasEmpty);
  if (wasEmpty && thread)
    thread->interrupt();
}

EBotMarketOrderDraft& DataService::createBrokerOrderDraft(EBotMarketOrder::Type type, double price)
{
  quint32 id = globalEntityManager.getNewEntityId<EBotMarketOrderDraft>();
  EBotMarketOrderDraft* eBotMarketOrderDraft = new EBotMarketOrderDraft(id, type, QDateTime::currentDateTime(), price);
  globalEntityManager.delegateEntity(*eBotMarketOrderDraft);
  return *eBotMarketOrderDraft;
}

void DataService::removeBrokerOrderDraft(EBotMarketOrderDraft &draft)
{
  globalEntityManager.removeEntity<EBotMarketOrderDraft>(draft.getId());
}

void DataService::submitBrokerOrderDraft(EBotMarketOrderDraft& draft)
{
  if(draft.getState() != EBotMarketOrder::State::draft)
    return;
  draft.setState(EBotMarketOrder::State::submitting);
  globalEntityManager.updatedEntity(draft);

  class SubmitBrokerOrderJob : public Job, public Event
  {
  public:
    SubmitBrokerOrderJob(quint64 draftId, EBotMarketOrder::Type type, double price, double amount) : 
      draftId(draftId), type(type), price(price), amount(amount), orderId(0) {}
  private:
    quint64 draftId;
    EBotMarketOrder::Type type;
    double price;
    double amount;
    quint64 orderId;
  private: // Job
    virtual bool execute(WorkerThread& workerThread)
    {
      if(!workerThread.connection.createBrokerOrder((meguco_user_broker_order_type)type, price, amount, orderId))
        return workerThread.addMessage(ELogMessage::Type::error, QString("Could not create broker order: %1").arg(workerThread.connection.getLastError())), false;
      return true;
    }
    virtual void handle(DataService& dataService)
    {
      if(!orderId) // todo: this is impossible
        return;
      EBotMarketOrderDraft* draft = dataService.globalEntityManager.getEntity<EBotMarketOrderDraft>(draftId);
      if(draft)
      {
        EBotMarketOrder* order = new EBotMarketOrder(orderId, *draft);
        dataService.globalEntityManager.delegateEntity(*order, *draft);
      }
    }
  };

  bool wasEmpty;
  jobQueue.append(new SubmitBrokerOrderJob(draft.getId(), draft.getType(), draft.getPrice(), draft.getAmount()), &wasEmpty);
  if(wasEmpty && thread)
    thread->interrupt();
}

void DataService::cancelBrokerOrder(EBotMarketOrder& order)
{
  if(order.getState() != EBotMarketOrder::State::open)
    return;
  order.setState(EBotMarketOrder::State::canceling);
  globalEntityManager.updatedEntity(order);

  class CancelBrokerOrderJob : public Job
  {
  public:
    CancelBrokerOrderJob(quint64 orderId) : orderId(orderId) {}
  private:
    quint64 orderId;
  private: // Job
    virtual bool execute(WorkerThread& workerThread)
    {
      bool controlResult;
      if(!workerThread.connection.controlBrokerOrder(orderId, meguco_user_broker_order_control_cancel, controlResult))
        return workerThread.addMessage(ELogMessage::Type::error, QString("Could not cancel broker order: %1").arg(workerThread.connection.getLastError())), false;
      return true;
    }
  };

  bool wasEmpty;
  jobQueue.append(new CancelBrokerOrderJob(order.getId()), &wasEmpty);
  if (wasEmpty && thread)
    thread->interrupt();
}

void DataService::updateBrokerOrder(EBotMarketOrder& order, double price, double amount)
{
  if(order.getState() != EBotMarketOrder::State::open)
    return;
  order.setState(EBotMarketOrder::State::updating);
  globalEntityManager.updatedEntity(order);

  class UpdateBrokerOrderJob : public Job
  {
  public:
    UpdateBrokerOrderJob(quint64 orderId, double price, double amount) : orderId(orderId), price(price), amount(amount) {}
  private:
    quint64 orderId;
    double price;
    double amount;
  private: // Job
    virtual bool execute(WorkerThread& workerThread)
    {
      bool controlResult;
      if(!workerThread.connection.updateBrokerOrder(orderId, price, amount, controlResult))
        workerThread.addMessage(ELogMessage::Type::error, QString("Could not control broker order: %1").arg(workerThread.connection.getLastError())), false;
      return true;
    }
  };

  bool wasEmpty;
  jobQueue.append(new UpdateBrokerOrderJob(order.getId(), price, amount), &wasEmpty);
  if(wasEmpty && thread)
    thread->interrupt();
}

/*private*//* void DataService::controlBrokerOrder(quint64 orderId, meguco_user_broker_order_control_code code, const void* data, size_t size)
{
  class ControlBrokerOrderJob : public Job
  {
  public:
    ControlBrokerOrderJob(quint64 orderId, meguco_user_broker_order_control_code code, const void* data, size_t size) : orderId(orderId), code(code), data((const char*)data, size) {}
  private:
    quint64 orderId;
    meguco_user_broker_order_control_code code;
    QByteArray data;
  private: // Job
    virtual bool execute(WorkerThread& workerThread)
    {
      if(!workerThread.connection.controlBrokerOrder(orderId, code, data.constData(), data.size()))
      {
        if(!workerThread.connection.getLastError().isEmpty())
          workerThread.addMessage(ELogMessage::Type::error, QString("Could not control broker order: %1").arg(workerThread.connection.getLastError()));
        return false;
      }
      return true;
    }
  };

  bool wasEmpty;
  jobQueue.append(new ControlBrokerOrderJob(orderId, code, data, size), &wasEmpty);
  if(wasEmpty && thread)
    thread->interrupt();
}
*/
void DataService::removeBrokerOrder(EBotMarketOrder& order)
{
  if(order.getState() != EBotMarketOrder::State::open)
    return;
  order.setState(EBotMarketOrder::State::removing);
  globalEntityManager.updatedEntity(order);

  class RemoveBrokerOrderJob : public Job
  {
  public:
    RemoveBrokerOrderJob(quint64 orderId) : orderId(orderId) {}
  private:
    quint64 orderId;
  private: // Job
    virtual bool execute(WorkerThread& workerThread)
    {
      bool controlResult;
      if(!workerThread.connection.controlBrokerOrder(orderId, meguco_user_broker_order_control_remove, controlResult))
        return workerThread.addMessage(ELogMessage::Type::error, QString("Could not remove broker order: %1").arg(workerThread.connection.getLastError())), false;
      return true;
    }
  };

  bool wasEmpty;
  jobQueue.append(new RemoveBrokerOrderJob(order.getId()), &wasEmpty);
  if(wasEmpty && thread)
    thread->interrupt();
}

void DataService::createSession(const QString& name, quint64 engineId, quint64 marketId)
{
  class CreateSessionJob : public Job
  {
  public:
    CreateSessionJob(const QString& name, quint64 engineId, quint64 marketId) : name(name), engineId(engineId), marketId(marketId) {}
  private:
    QString name;
    quint64 engineId;
    quint64 marketId;
  private: // Job
    virtual bool execute(WorkerThread& workerThread)
    {
      if(!workerThread.connection.createSession(name, engineId, marketId))
        return workerThread.addMessage(ELogMessage::Type::error, QString("Could not cancel broker order: %1").arg(workerThread.connection.getLastError())), false;
      return true;
    }
  };

  bool wasEmpty;
  jobQueue.append(new CreateSessionJob(name, engineId, marketId), &wasEmpty);
  if(wasEmpty && thread)
    thread->interrupt();
}

void DataService::removeSession(quint32 sessionId)
{
  class RemoveSessionJob : public Job
  {
  public:
    RemoveSessionJob(quint32 sessionId) : sessionId(sessionId) {}
  private:
    quint32 sessionId;
  private: // Job
    virtual bool execute(WorkerThread& workerThread)
    {
      if(!workerThread.connection.removeSession(sessionId))
        return workerThread.addMessage(ELogMessage::Type::error, QString("Could not remove session: %1").arg(workerThread.connection.getLastError())), false;
      return true;
    }
  };

  bool wasEmpty;
  jobQueue.append(new RemoveSessionJob(sessionId), &wasEmpty);
  if(wasEmpty && thread)
    thread->interrupt();
}

void DataService::stopSession(quint32 sessionId)
{

  controlSession(sessionId, meguco_user_session_control_stop);
}

void DataService::startSessionSimulation(quint32 sessionId)
{
  controlSession(sessionId, meguco_user_session_control_start_simulation);
}

void DataService::startSession(quint32 sessionId)
{
  controlSession(sessionId, meguco_user_session_control_start_live);
}

void DataService::selectSession(quint32 sessionId)
{
  class SelectSessionJob : public Job, public Event
  {
  public:
    SelectSessionJob(quint32 sessionId) : sessionId(sessionId) {}
  private:
    quint32 sessionId;
  private: // Job
    virtual bool execute(WorkerThread& workerThread)
    {
      if(!workerThread.connection.selectSession(sessionId))
        return workerThread.addMessage(ELogMessage::Type::error, QString("Could not select session: %1").arg(workerThread.connection.getLastError())), false;
      return true;
    }
  private: // Event
    virtual void handle(DataService& dataService)
    {
      Entity::Manager& globalEntityManager = dataService.globalEntityManager;
      EDataService* eDataService = globalEntityManager.getEntity<EDataService>(0);
      eDataService->setSelectedSessionId(sessionId);
      globalEntityManager.updatedEntity(*eDataService);
    }
  };

  bool wasEmpty;
  jobQueue.append(new SelectSessionJob(sessionId), &wasEmpty);
  if(wasEmpty && thread)
    thread->interrupt();
}

EBotSessionItemDraft& DataService::createSessionAssetDraft(EBotSessionItem::Type type, double flipPrice)
{
  quint32 id = globalEntityManager.getNewEntityId<EBotSessionItemDraft>();
  EBotSessionItemDraft* eBotSessionItemDraft = new EBotSessionItemDraft(id, type, QDateTime::currentDateTime(), flipPrice);
  globalEntityManager.delegateEntity(*eBotSessionItemDraft);
  return *eBotSessionItemDraft;
}

void DataService::submitSessionAssetDraft(EBotSessionItemDraft& draft)
{
  if(draft.getState() != EBotSessionItemDraft::State::draft)
    return;
  draft.setState(EBotSessionItemDraft::State::submitting);
  globalEntityManager.updatedEntity(draft);

  class SubmitSessionAssetJob : public Job
  {
  public:
    SubmitSessionAssetJob(EBotSessionItem::Type type, double balanceComm, double balanceBase, double flipPrice) : 
      type(type), balanceComm(balanceComm), balanceBase(balanceBase), flipPrice(flipPrice) {}
  private:
    EBotSessionItem::Type type;
    double balanceComm;
    double balanceBase;
    double flipPrice;
  private: // Job
    virtual bool execute(WorkerThread& workerThread)
    {
      quint64 assetId;
      if(!workerThread.connection.createSessionAsset((meguco_user_session_asset_type)type, balanceComm, balanceBase, flipPrice, assetId))
        return workerThread.addMessage(ELogMessage::Type::error, QString("Could not create session asset: %1").arg(workerThread.connection.getLastError())), false;
      return true;
    }
  };

  bool wasEmpty;
  jobQueue.append(new SubmitSessionAssetJob(draft.getType(), draft.getBalanceComm(), draft.getBalanceBase(), draft.getFlipPrice()), &wasEmpty);
  if(wasEmpty && thread)
    thread->interrupt();
}

void DataService::updateSessionAsset(EBotSessionItem& asset, double flipPrice)
{
  if(asset.getState() != EBotSessionItem::State::waitBuy && asset.getState() != EBotSessionItem::State::waitSell)
    return;
  asset.setState(EBotSessionItem::State::updating);
  globalEntityManager.updatedEntity(asset);

  class UpdateSessionAssetJob : public Job
  {
  public:
    UpdateSessionAssetJob(quint64 assetId, double flipPrice) : assetId(assetId), flipPrice(flipPrice) {}
  private:
    quint64 assetId;
    double flipPrice;
  private: // Job
    virtual bool execute(WorkerThread& workerThread)
    {
      bool controlResult;
      if(!workerThread.connection.updateSessionAsset(assetId, flipPrice, controlResult))
        return workerThread.addMessage(ELogMessage::Type::error, QString("Could not control broker order: %1").arg(workerThread.connection.getLastError())), false;
      return true;
    }
  };

  bool wasEmpty;
  jobQueue.append(new UpdateSessionAssetJob(asset.getId(), flipPrice), &wasEmpty);
  if(wasEmpty && thread)
    thread->interrupt();
}

void DataService::removeSessionAsset(EBotSessionItem& asset)
{
  if(asset.getState() != EBotSessionItem::State::waitBuy && asset.getState() != EBotSessionItem::State::waitSell)
    return;
  asset.setState(EBotSessionItem::State::removing);
  globalEntityManager.updatedEntity(asset);

  class RemoveSessionAssetJob : public Job
  {
  public:
    RemoveSessionAssetJob(quint64 assetId) : assetId(assetId) {}
  private:
    quint64 assetId;
  private: // Job
    virtual bool execute(WorkerThread& workerThread)
    {
      bool controlResult;
      if(!workerThread.connection.controlSessionAsset(assetId, meguco_user_session_asset_control_remove, controlResult))
        return workerThread.addMessage(ELogMessage::Type::error, QString("Could not remove session asset: %1").arg(workerThread.connection.getLastError())), false;
      return true;
    }
  };

  bool wasEmpty;
  jobQueue.append(new RemoveSessionAssetJob(asset.getId()), &wasEmpty);
  if(wasEmpty && thread)
    thread->interrupt();
}

void DataService::removeSessionAssetDraft(EBotSessionItemDraft& draft)
{
  globalEntityManager.removeEntity<EBotSessionItemDraft>(draft.getId());
}

void DataService::updateSessionProperty(EBotSessionProperty& property, const QString& value)
{
  class UpdateSessionPropertyJob : public Job
  {
  public:
    UpdateSessionPropertyJob(quint64 propertyId, const QString& value) : propertyId(propertyId), value(value) {}
  private:
    quint64 propertyId;
    QString value;
  private: // Job
    virtual bool execute(WorkerThread& workerThread)
    {
      QByteArray valueData = value.toUtf8();
      if(!workerThread.connection.updateSessionProperty(propertyId, value))
        return workerThread.addMessage(ELogMessage::Type::error, QString("Could not control session property: %1").arg(workerThread.connection.getLastError())), false;
      return true;
    }
  };

  bool wasEmpty;
  jobQueue.append(new UpdateSessionPropertyJob(property.getId(), value), &wasEmpty);
  if(wasEmpty && thread)
    thread->interrupt();
}

/*private*//* void DataService::controlBroker(meguco_user_broker_control_code code)
{
  class ControlBrokerJob : public Job
  {
  public:
    ControlBrokerJob(meguco_user_broker_control_code code) : code(code) {}
  private:
    meguco_user_broker_control_code code;
  private: // Job
    virtual bool execute(WorkerThread& workerThread)
    {
      if(!workerThread.connection.controlBroker(code))
      {
        if(!workerThread.connection.getLastError().isEmpty())
          workerThread.addMessage(ELogMessage::Type::error, QString("Could not control broker: %1").arg(workerThread.connection.getLastError()));
        return false;
      }
      return true;
    }
  };

  bool wasEmpty;
  jobQueue.append(new ControlBrokerJob(code), &wasEmpty);
  if(wasEmpty && thread)
    thread->interrupt();
}
*/
/*private*/ void DataService::controlSession(quint32 sessionId, meguco_user_session_control_code code)
{
  class ControlSessionJob : public Job
  {
  public:
    ControlSessionJob(quint32 sessionId, meguco_user_session_control_code code) : sessionId(sessionId), code(code) {}
  private:
    quint32 sessionId;
    meguco_user_session_control_code code;
  private: // Job
    virtual bool execute(WorkerThread& workerThread)
    {
      if(!workerThread.connection.controlSession(sessionId, code))
      {
        if(!workerThread.connection.getLastError().isEmpty())
          workerThread.addMessage(ELogMessage::Type::error, QString("Could not control session: %1").arg(workerThread.connection.getLastError()));
        return false;
      }
      return true;
    }
  };

  bool wasEmpty;
  jobQueue.append(new ControlSessionJob(sessionId, code), &wasEmpty);
  if(wasEmpty && thread)
    thread->interrupt();
}
