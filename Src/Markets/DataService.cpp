
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
    SubscriptionJob(const QString& channelName, quint32 channelId, quint64 lastReceivedTradeId) : channelName(channelName), channelId(channelId), lastReceivedTradeId(lastReceivedTradeId), eTradeData(0) {}
    ~SubscriptionJob() {delete eTradeData;}
  private:
    QString channelName;
    quint32 channelId;
    quint64 lastReceivedTradeId;
    EDataTradeData* eTradeData;
  private: // Job
    virtual bool execute(WorkerThread& workerThread)
    {
      WorkerThread::SubscriptionData& data = workerThread.subscriptionData[channelId];
      data.channelName = channelName;
      if(data.eTradeData)
        delete data.eTradeData;
      data.eTradeData = new EDataTradeData;

      if(!workerThread.connection.subscribe(channelId, lastReceivedTradeId))
        return workerThread.addMessage(ELogMessage::Type::error, QString("Could not subscribe to channel %1: %2").arg(channelName, workerThread.connection.getLastError())), false;
      
      eTradeData = data.eTradeData;
      data.eTradeData = 0;
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
      if(!workerThread.connection.unsubscribe(channelId))
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
  class LogMessageEvent : public Event
  {
  public:
    LogMessageEvent(ELogMessage::Type type, const QString& message) : type(type), message(message) {}
  private:
    ELogMessage::Type type;
    QString message;
  public: // Event
    virtual void handle(DataService& dataService)
    {
        dataService.addLogMessage(type, message);
    }
  };
  bool wasEmpty;
  eventQueue.append(new LogMessageEvent(type, message), &wasEmpty);
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

  // load channel list
  if(!connection.loadChannelList())
    return addMessage(ELogMessage::Type::error, QString("Could not load channel list: %1").arg(connection.getLastError()));

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

void DataService::WorkerThread::receivedChannelInfo(quint32 channelId, const QString& channelName)
{
  class ChannelInfoEvent : public Event
  {
  public:
    ChannelInfoEvent(quint64 channelId, const QString& channelName) : channelId(channelId), channelName(channelName) {}
  private:
    quint32 channelId;
    QString channelName;
  public: // Event
    virtual void handle(DataService& dataService)
    {
      int firstSlash = channelName.indexOf('/');
      int secondSlash = channelName.indexOf('/', firstSlash + 1);
      QString baseCurrency = channelName.mid(secondSlash + 1);
      QString commCurrency = channelName.mid(firstSlash + 1, secondSlash - (firstSlash + 1));

      Entity::Manager& globalEntityManager = dataService.globalEntityManager;
      EDataMarket* eDataMarket = new EDataMarket(channelId, channelName, baseCurrency, commCurrency);
      globalEntityManager.delegateEntity(*eDataMarket);
    }
  };

  bool wasEmpty;
  eventQueue.append(new ChannelInfoEvent(channelId, channelName), &wasEmpty);
  if(wasEmpty)
    QTimer::singleShot(0, &dataService, SLOT(handleEvents()));
  //information(QString("Found channel %1.").arg(channelName));
}

void DataService::WorkerThread::receivedSubscribeResponse(quint32 channelId)
{
  QHash<quint32, SubscriptionData>::Iterator it =  subscriptionData.find(channelId);
  if(it == subscriptionData.end())
    return;
  SubscriptionData& data = it.value();

  class SubscribeResponseEvent : public Event
  {
  public:
    SubscribeResponseEvent(quint64 channelId, const QString& channelName) : channelId(channelId), channelName(channelName) {}
  private:
    quint32 channelId;
    QString channelName;
  private: // Event
    virtual void handle(DataService& dataService)
    {
      Entity::Manager* channelEntityManager = dataService.getSubscription(channelName);
      if(channelEntityManager)
      {
        dataService.activeSubscriptions[channelId] = channelEntityManager;
        EDataSubscription* eDataSubscription = channelEntityManager->getEntity<EDataSubscription>(0);
        eDataSubscription->setState(EDataSubscription::State::loading);
        channelEntityManager->updatedEntity(*eDataSubscription);
      }
    }
  };

  bool wasEmpty;
  eventQueue.append(new SubscribeResponseEvent(channelId, data.channelName), &wasEmpty);
  if(wasEmpty)
    QTimer::singleShot(0, &dataService, SLOT(handleEvents()));
}

void DataService::WorkerThread::receivedTrade(quint32 channelId, const meguco_trade_entity& tradeData)
{
  EDataTradeData::Trade trade;
  trade.id = tradeData.entity.id;
  trade.time = tradeData.entity.time;
  trade.price = tradeData.price;
  trade.amount = tradeData.amount;

  SubscriptionData& data = subscriptionData[channelId];
  if(data.eTradeData)
    data.eTradeData->addTrade(trade);
  else // live trade
  {
    class AddTradeEvent : public Event
    {
    public:
      AddTradeEvent(quint32 channelId, const EDataTradeData::Trade& trade) : channelId(channelId), trade(trade) {}
    private:
      quint32 channelId;
      EDataTradeData::Trade trade;
    private: // Event
      virtual void handle(DataService& dataService)
      {
        QHash<quint32, Entity::Manager*>::ConstIterator it = dataService.activeSubscriptions.find(channelId);
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
    eventQueue.append(new AddTradeEvent(channelId, trade), &wasEmpty);
    if(wasEmpty)
      QTimer::singleShot(0, &dataService, SLOT(handleEvents()));
  }
}

void DataService::WorkerThread::receivedTicker(quint32 channelId, const meguco_ticker_entity& ticker)
{
  class AddTickerEvent : public Event
  {
  public:
    AddTickerEvent(quint64 channelId, const meguco_ticker_entity& ticker) : channelId(channelId), ticker(ticker) {}
  private:
    quint64 channelId;
    meguco_ticker_entity ticker;
  private: // Event
    virtual void handle(DataService& dataService)
    {
      QHash<quint32, Entity::Manager*>::ConstIterator it = dataService.activeSubscriptions.find(channelId);
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
  eventQueue.append(new AddTickerEvent(channelId, ticker), &wasEmpty);
  if(wasEmpty)
    QTimer::singleShot(0, &dataService, SLOT(handleEvents()));
}
