
#include "stdafx.h"

DataService::DataService(Entity::Manager& globalEntityManager) :
  globalEntityManager(globalEntityManager), thread(0), isConnected(false) {}

DataService::~DataService()
{
  stop();
}

void DataService::start(const QString& server)
{
  if(thread)
  {
     if(server == thread->getServer())
       return;
     stop();
     Q_ASSERT(!thread);
  }

  thread = new WorkerThread(*this, eventQueue, jobQueue, server);
  thread->start();
}

void DataService::stop()
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

void DataService::subscribe(const QString& channel, Entity::Manager& channelEntityManager)
{
  if(subscriptions.contains(channel))
    return;
  subscriptions.insert(channel, &channelEntityManager);
  if(!isConnected)
    return;

  class SubscriptionJob : public Job
  {
  public:
    SubscriptionJob(const QString& channel, quint64 lastReceivedTradeId) : channel(channel), lastReceivedTradeId(lastReceivedTradeId) {}
  private:
    QString channel;
    quint64 lastReceivedTradeId;
  private: // Job
    virtual bool execute(WorkerThread& workerThread)
    {
      if(!workerThread.connection.subscribe(channel, lastReceivedTradeId))
        return false;
      return true;
    }
  };

  addLogMessage(ELogMessage::Type::information, QString("Subscribing to channel %1...").arg(channel));
  EDataTradeData* eDataTradeData = channelEntityManager.getEntity<EDataTradeData>(0);
  quint64 lastReceivedTradeId = 0;
  if(eDataTradeData)
  {
    const QList<DataProtocol::Trade>& data = eDataTradeData->getData();
    if(!data.isEmpty())
      lastReceivedTradeId = data.back().id;
  }
  jobQueue.append(new SubscriptionJob(channel, lastReceivedTradeId));
  if(thread)
    thread->interrupt();
  EDataSubscription* eDataSubscription = channelEntityManager.getEntity<EDataSubscription>(0);
  eDataSubscription->setState(EDataSubscription::State::subscribing);
  channelEntityManager.updatedEntity(*eDataSubscription);
}

void DataService::unsubscribe(const QString& channel)
{
  QHash<QString, Entity::Manager*>::Iterator it = subscriptions.find(channel);
  if(it == subscriptions.end())
    return;
  subscriptions.erase(it);
  if(!isConnected)
    return;

  class UnsubscriptionJob : public Job
  {
  public:
    UnsubscriptionJob(const QString& channel) : channel(channel) {}
  private:
    QString channel;
  private: // Job
    virtual bool execute(WorkerThread& workerThread)
    {
      if(!workerThread.connection.unsubscribe(channel))
        return false;
      return true;
    }
  };

  addLogMessage(ELogMessage::Type::information, QString("Unsubscribing from channel %1...").arg(channel));
  jobQueue.append(new UnsubscriptionJob(channel));
  if(thread)
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
  eventQueue.append(new LogMessageEvent(type, message));
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
        for(QHash<quint64, Entity::Manager*>::ConstIterator i = dataService.activeSubscriptions.begin(), end = dataService.activeSubscriptions.end(); i != end; ++i)
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
      {
        globalEntityManager.removeAll<EDataMarket>();
      }
      globalEntityManager.updatedEntity(*eDataService);
    }
  };

  eventQueue.append(new SetStateEvent(state));
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
  if(!connection.connect(addr.size() > 0 ? addr[0] : QString(), addr.size() > 1 ? addr[1].toULong() : 0))
  {
    addMessage(ELogMessage::Type::error, QString("Could not connect to data service: %1").arg(connection.getLastError()));
    return;
  }
  addMessage(ELogMessage::Type::information, "Connected to data service.");
  setState(EDataService::State::connected);

  // load channel list
  if(!connection.loadChannelList())
    goto error;

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
  addMessage(ELogMessage::Type::error, QString("Lost connection to data service: %1").arg(connection.getLastError()));
}

void DataService::WorkerThread::run()
{
  while(!canceled)
  {
    setState(EDataService::State::connecting);
    process();
    setState(EDataService::State::offline);

    qDeleteAll(replayedTrades);
    replayedTrades.clear();

    if(canceled)
      return;
    sleep(10);
  }
}

void DataService::WorkerThread::receivedChannelInfo(const QString& channelName)
{
  class ChannelInfoEvent : public Event
  {
  public:
    ChannelInfoEvent(const QString& channelName) : channelName(channelName) {}
  private:
    QString channelName;
  public: // Event
    virtual void handle(DataService& dataService)
    {
      QString baseCurrency = channelName.mid(channelName.lastIndexOf('/') + 1);
      QString commCurrency("BTC"); // todo: receive commCurrency from data server

      Entity::Manager& globalEntityManager = dataService.globalEntityManager;
      quint32 entityId = globalEntityManager.getNewEntityId<EDataMarket>();
      EDataMarket* eDataMarket = new EDataMarket(entityId, channelName, baseCurrency, commCurrency);
      globalEntityManager.delegateEntity(*eDataMarket);

      //Entity::Manager* channelEntityManager =  dataService.getSubscription(channelName);
      //if(channelEntityManager)
      //{
      //  EDataSubscription* eDataSubscription = channelEntityManager->getEntity<EDataSubscription>(0);
      //  if(eDataSubscription)
      //  {
      //    if(eDataSubscription->getBaseCurrency() != baseCurrency || eDataSubscription->getCommCurrency() != commCurrency)
      //    {
      //      // update
      //    }
      //  }
      //}
    }
  };

  eventQueue.append(new ChannelInfoEvent(channelName));
  QTimer::singleShot(0, &dataService, SLOT(handleEvents()));
  //information(QString("Found channel %1.").arg(channelName));
}

void DataService::WorkerThread::receivedSubscribeResponse(const QString& channelName, quint64 channelId)
{
  class SubscribeResponseEvent : public Event
  {
  public:
    SubscribeResponseEvent(const QString& channelName, quint64 channelId) : channelName(channelName), channelId(channelId) {}
  private:
    QString channelName;
    quint64 channelId;
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
        dataService.addLogMessage(ELogMessage::Type::information, QString("Subscribed to channel %1.").arg(channelName));
      }
    }
  };

  eventQueue.append(new SubscribeResponseEvent(channelName, channelId));
  QTimer::singleShot(0, &dataService, SLOT(handleEvents()));
  replayedTrades.remove(channelId);
}

void DataService::WorkerThread::receivedUnsubscribeResponse(const QString& channelName, quint64 channelId)
{
  class UnsubscribeResponseEvent : public Event
  {
  public:
    UnsubscribeResponseEvent(const QString& channelName, quint64 channelId) : channelName(channelName), channelId(channelId) {}
  private:
    QString channelName;
    quint64 channelId;
  private: // Event
    virtual void handle(DataService& dataService)
    {
      QHash<quint64, Entity::Manager*>::Iterator it = dataService.activeSubscriptions.find(channelId);
      if(it == dataService.activeSubscriptions.end())
        return;
      Entity::Manager* channelEntityManager = it.value();
      dataService.activeSubscriptions.erase(it);
      EDataSubscription* eDataSubscription = channelEntityManager->getEntity<EDataSubscription>(0);
      eDataSubscription->setState(EDataSubscription::State::unsubscribed);
      dataService.addLogMessage(ELogMessage::Type::information, QString("Unsubscribed from channel %1.").arg(channelName));
    }
  };

  eventQueue.append(new UnsubscribeResponseEvent(channelName, channelId));
  QTimer::singleShot(0, &dataService, SLOT(handleEvents()));
  replayedTrades.remove(channelId);
}

void DataService::WorkerThread::receivedTrade(quint64 channelId, const DataProtocol::Trade& trade)
{
  if(trade.flags & DataProtocol::replayedFlag)
  {
    QHash<quint64, EDataTradeData*>::Iterator it = replayedTrades.find(channelId);
    EDataTradeData* tradeData;
    if(it == replayedTrades.end())
    {
      tradeData = new EDataTradeData;
      replayedTrades.insert(channelId, tradeData);
    }
    else
      tradeData = it.value();
    tradeData->addTrade(trade);

    if(trade.flags & DataProtocol::syncFlag)
    {
      class SetTradesEvent : public Event
      {
      public:
        SetTradesEvent(quint64 channelId, EDataTradeData* tradeData) : channelId(channelId), tradeData(tradeData) {}
      private:
        quint64 channelId;
        EDataTradeData* tradeData;
      private: // Event
        virtual void handle(DataService& dataService)
        {
          QHash<quint64, Entity::Manager*>::ConstIterator it = dataService.activeSubscriptions.find(channelId);
          if(it == dataService.activeSubscriptions.end())
          {
            delete tradeData;
            return;
          }
          Entity::Manager* channelEntityManager = it.value();
          if(channelEntityManager)
          {
            channelEntityManager->delegateEntity(*tradeData);
            EDataSubscription* eDataSubscription = channelEntityManager->getEntity<EDataSubscription>(0);
            eDataSubscription->setState(EDataSubscription::State::subscribed);
            channelEntityManager->updatedEntity(*eDataSubscription);

            //const QList<DataProtocol::Trade>& trades = tradeData->getData();
            //for(QList<DataProtocol::Trade>::ConstIterator i = trades.begin() + (trades.size() > 100 ? (trades.size() - 100) : 0), end = trades.end(); i != end; ++i)
            //{
            //  quint32 entityId = channelEntityManager->getNewEntityId<EDataTrade>();
            //  channelEntityManager->delegateEntity(*new EDataTrade(entityId, *i));
            //}
          }
          else
            delete tradeData;
        }
      };

      SetTradesEvent* setTradesEvent = new SetTradesEvent(channelId, tradeData);
      eventQueue.append(setTradesEvent);
      QTimer::singleShot(0, &dataService, SLOT(handleEvents()));
      replayedTrades.remove(channelId);
    }
  }
  else
  {
    class AddTradeEvent : public Event
    {
    public:
      AddTradeEvent(quint64 channelId, const DataProtocol::Trade& trade) : channelId(channelId), trade(trade) {}
    private:
      quint64 channelId;
      DataProtocol::Trade trade;
    private: // Event
      virtual void handle(DataService& dataService)
      {
        QHash<quint64, Entity::Manager*>::ConstIterator it = dataService.activeSubscriptions.find(channelId);
        if(it == dataService.activeSubscriptions.end())
          return;
        Entity::Manager* channelEntityManager = it.value();
        //quint32 entityId = channelEntityManager->getNewEntityId<EDataTrade>();
        //channelEntityManager->delegateEntity(*new EDataTrade(entityId, trade));

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

    eventQueue.append(new AddTradeEvent(channelId, trade));
    QTimer::singleShot(0, &dataService, SLOT(handleEvents()));
  }
}

void DataService::WorkerThread::receivedTicker(quint64 channelId, const DataProtocol::Ticker& ticker)
{
  class AddTickerEvent : public Event
  {
  public:
    AddTickerEvent(quint64 channelId, const DataProtocol::Ticker& ticker) : channelId(channelId), ticker(ticker) {}
  private:
    quint64 channelId;
    DataProtocol::Ticker ticker;
  private: // Event
    virtual void handle(DataService& dataService)
    {
      QHash<quint64, Entity::Manager*>::ConstIterator it = dataService.activeSubscriptions.find(channelId);
      if(it == dataService.activeSubscriptions.end())
        return;
      Entity::Manager* channelEntityManager = it.value();
      EDataTickerData* eDataTickerData = channelEntityManager->getEntity<EDataTickerData>(0);
      if(eDataTickerData)
      {
        eDataTickerData->setData(ticker);
        channelEntityManager->updatedEntity(*eDataTickerData);
      }
      else
      {
        eDataTickerData = new EDataTickerData(ticker);
        channelEntityManager->delegateEntity(*eDataTickerData);
      }
    }
  };

  eventQueue.append(new AddTickerEvent(channelId, ticker));
  QTimer::singleShot(0, &dataService, SLOT(handleEvents()));
}

void DataService::WorkerThread::receivedErrorResponse(const QString& message)
{
  addMessage(ELogMessage::Type::error, message);
}
