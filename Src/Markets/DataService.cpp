
#include "stdafx.h"

DataService::DataService(DataModel& dataModel, Entity::Manager& entityManager) :
  dataModel(dataModel), entityManager(entityManager), thread(0), isConnected(false) {}

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

void DataService::subscribe(const QString& channel)
{
  if(subscriptions.contains(channel))
    return;
  subscriptions.insert(channel);
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
  PublicDataModel& publicDataModel = dataModel.getDataChannel(channel);
  jobQueue.append(new SubscriptionJob(channel, publicDataModel.getLastReceivedTradeId()));
  if(thread)
    thread->interrupt();
  publicDataModel.setState(PublicDataModel::State::connecting);
}

void DataService::unsubscribe(const QString& channel)
{
  QSet<QString>::Iterator it = subscriptions.find(channel);
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
  entityManager.delegateEntity(*logMessage);
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

void DataService::WorkerThread::setState(PublicDataModel::State state)
{
  class SetStateEvent : public Event
  {
  public:
    SetStateEvent(PublicDataModel::State state) : state(state) {}
  private:
    PublicDataModel::State state;
  private: // Event
    virtual void handle(DataService& dataService)
    {
      if(state == PublicDataModel::State::connected)
      {
        dataService.isConnected = true;
        QSet<QString> subscriptions;
        subscriptions.swap(dataService.subscriptions);
        foreach(const QString& channel, subscriptions)
          dataService.subscribe(channel);
      }
      else if(state == PublicDataModel::State::offline)
      {
        dataService.isConnected = false;
        for(QMap<QString, PublicDataModel*>::ConstIterator i = dataService.dataModel.getDataChannels().begin(), end = dataService.dataModel.getDataChannels().end(); i != end; ++i)
        {
          PublicDataModel* publicDataModel = i.value();
          if(publicDataModel)
            publicDataModel->setState(PublicDataModel::State::offline);
        }
        dataService.activeSubscriptions.clear();
      }
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
  setState(PublicDataModel::State::connected);

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
    setState(PublicDataModel::State::connecting);
    process();
    setState(PublicDataModel::State::offline);
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
      dataService.dataModel.addDataChannel(channelName);
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
      DataModel& dataModel = dataService.dataModel;
      PublicDataModel& publicDataModel = dataModel.getDataChannel(channelName);
      dataService.activeSubscriptions[channelId] = &publicDataModel;
      publicDataModel.setState(PublicDataModel::State::loading);
      dataService.addLogMessage(ELogMessage::Type::information, QString("Subscribed to channel %1.").arg(channelName));
    }
  };

  eventQueue.append(new SubscribeResponseEvent(channelName, channelId));
  QTimer::singleShot(0, &dataService, SLOT(handleEvents()));
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
      DataModel& dataModel = dataService.dataModel;
      PublicDataModel& publicDataModel = dataModel.getDataChannel(channelName);
      dataService.activeSubscriptions.remove(channelId);
      if(publicDataModel.getState() == PublicDataModel::State::connected)
        publicDataModel.setState(PublicDataModel::State::offline);
      dataService.addLogMessage(ELogMessage::Type::information, QString("Unsubscribed from channel %1.").arg(channelName));
    }
  };

  eventQueue.append(new UnsubscribeResponseEvent(channelName, channelId));
  QTimer::singleShot(0, &dataService, SLOT(handleEvents()));
}

void DataService::WorkerThread::receivedTrade(quint64 channelId, const DataProtocol::Trade& trade)
{
  if(trade.flags & DataProtocol::replayedFlag)
  {
    QList<DataProtocol::Trade>& trades = replayedTrades[channelId];
    trades.append(trade);

    if(trade.flags & DataProtocol::syncFlag)
    {
      class SetTradesEvent : public Event
      {
      public:
        SetTradesEvent(quint64 channelId) : channelId(channelId) {}
        QList<DataProtocol::Trade> trades;
      private:
        quint64 channelId;
      private: // Event
        virtual void handle(DataService& dataService)
        {
          PublicDataModel* publicDataModel = dataService.activeSubscriptions[channelId];
          if(publicDataModel)
          {
            publicDataModel->setTrades(trades);
            publicDataModel->setState(PublicDataModel::State::connected);
          }
        }
      };

      SetTradesEvent* setTradesEvent = new SetTradesEvent(channelId);
      setTradesEvent->trades.swap(trades);
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
        PublicDataModel* publicDataModel = dataService.activeSubscriptions[channelId];
        if(publicDataModel)
          publicDataModel->addTrade(trade);
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
      PublicDataModel* publicDataModel = dataService.activeSubscriptions[channelId];
      if(publicDataModel)
        publicDataModel->addTicker(ticker.time / 1000ULL, ticker.bid, ticker.ask);
    }
  };

  eventQueue.append(new AddTickerEvent(channelId, ticker));
  QTimer::singleShot(0, &dataService, SLOT(handleEvents()));
}

void DataService::WorkerThread::receivedErrorResponse(const QString& message)
{
  addMessage(ELogMessage::Type::error, message);
}
