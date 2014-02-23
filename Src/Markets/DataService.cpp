
#include "stdafx.h"

DataService::DataService(DataModel& dataModel) :
  dataModel(dataModel), thread(0)
{
}

DataService::~DataService()
{
  stop();

  qDeleteAll(actionQueue.getAll());
}

void DataService::start()
{
  if(thread)
    return;

  struct MyWorkerThread : public WorkerThread, public DataConnection::Callback
  {
    MyWorkerThread(DataService& dataService, JobQueue<Action*>& actionQueue, JobQueue<Action*>& subscriptionQueue) :
      dataService(dataService), actionQueue(actionQueue), subscriptionQueue(subscriptionQueue), canceled(false) {}

    virtual void run()
    {
      while(!canceled)
      {
        actionQueue.append(new SetStateAction(PublicDataModel::State::connecting));
        QTimer::singleShot(0, &dataService, SLOT(executeActions()));
        process();
        actionQueue.append(new SetStateAction(PublicDataModel::State::offline));
        QTimer::singleShot(0, &dataService, SLOT(executeActions()));
        if(canceled)
          return;
        sleep(10);
      }
    }

    virtual void interrupt()
    {
      connection.interrupt();
    }

    virtual void receivedChannelInfo(const QString& channelName)
    {
      actionQueue.append(new ChannelInfoAction(channelName));
      QTimer::singleShot(0, &dataService, SLOT(executeActions()));
      information(QString("Found channel %1.").arg(channelName));
    }

    virtual void receivedSubscribeResponse(const QString& channelName, quint64 channelId)
    {
      actionQueue.append(new SubscribeResponseAction(Action::Type::subscribeResponse, channelName, channelId));
      QTimer::singleShot(0, &dataService, SLOT(executeActions()));
    }

    virtual void receivedUnsubscribeResponse(const QString& channelName, quint64 channelId)
    {
      actionQueue.append(new SubscribeResponseAction(Action::Type::unsubscribeResponse, channelName, channelId));
      QTimer::singleShot(0, &dataService, SLOT(executeActions()));
    }

    virtual void receivedTrade(quint64 channelId, const DataProtocol::Trade& trade)
    {
      actionQueue.append(new AddTradeAction(channelId, trade));
      QTimer::singleShot(0, &dataService, SLOT(executeActions()));
    }

    virtual void receivedErrorResponse(const QString& message)
    {
      actionQueue.append(new LogMessageAction(LogModel::Type::error, message));
      QTimer::singleShot(0, &dataService, SLOT(executeActions()));
    }

    virtual void information(const QString& message)
    {
      actionQueue.append(new LogMessageAction(LogModel::Type::information, message));
      QTimer::singleShot(0, &dataService, SLOT(executeActions()));
    }

    void connected()
    {
      actionQueue.append(new SetStateAction(PublicDataModel::State::connected));
      QTimer::singleShot(0, &dataService, SLOT(executeActions()));
    }

    void process()
    {
      information("Connecting to data service...");

      // create connection
      if(!connection.connect())
      {
        receivedErrorResponse(QString("Could not connect to data service: %1").arg(connection.getLastError()));
        return;
      }
      information("Connected to data service.");
      connected();

      // load channel list
      if(!connection.loadChannelList())
        goto error;

      // subscribe to channels
      foreach(const QString& channel, subscriptions)
        if(!connection.subscribe(channel))
          goto error;

      // loop
      Action* action;
      for(;;)
      {
        while(subscriptionQueue.get(action, 0))
        {
          if(!action)
          {
            canceled = true;
            return;
          }
          switch(action->type)
          {
          case Action::Type::subscribe:
            {
              SubscriptionAction* subscriptionAction = (SubscriptionAction*)action;
              if(!subscriptions.contains(subscriptionAction->channel))
              {
                subscriptions.insert(subscriptionAction->channel);
                if(!connection.subscribe(subscriptionAction->channel))
                  goto error;
              }
            }
            break;
          case Action::Type::unsubscribe:
            {
              SubscriptionAction* subscriptionAction = (SubscriptionAction*)action;
              if(subscriptions.contains(subscriptionAction->channel))
              {
                subscriptions.remove(subscriptionAction->channel);
                if(!connection.unsubscribe(subscriptionAction->channel))
                  goto error;
              }
            }
            break;
          default:
            break;
          }
          delete action;
        }

        if(!connection.process(*this))
          break;
      }

    error:
      receivedErrorResponse(QString("Lost connection to data service: %1").arg(connection.getLastError()));
    }

    DataService& dataService;
    JobQueue<Action*>& actionQueue;
    JobQueue<Action*>& subscriptionQueue;
    DataConnection connection;
    QSet<QString> subscriptions;
    bool canceled;
  };

  thread = new MyWorkerThread(*this, actionQueue, subscriptionQueue);
  thread->start();
}

void DataService::stop()
{
  if(!thread)
    return;

  subscriptionQueue.append(0); // cancel thread message
  thread->interrupt();
  thread->wait();
  delete thread;
  thread = 0;

  executeActions(); // better than qDeleteAll(actionQueue.getAll()); ;)
  qDeleteAll(subscriptionQueue.getAll());
}

void DataService::subscribe(const QString& channel)
{
  PublicDataModel& publicDataModel = dataModel.getDataChannel(channel);
  if(publicDataModel.getState() != PublicDataModel::State::offline)
    return;
  subscriptionQueue.append(new SubscriptionAction(Action::Type::subscribe, channel));
  if(thread)
    thread->interrupt();
  publicDataModel.setState(PublicDataModel::State::connecting);
  dataModel.logModel.addMessage(LogModel::Type::information, QString("Subscribing to channel %1...").arg(channel));
}

void DataService::unsubscribe(const QString& channel)
{
  dataModel.logModel.addMessage(LogModel::Type::information, QString("Unsubscribing from channel %1...").arg(channel));
  subscriptionQueue.append(new SubscriptionAction(Action::Type::unsubscribe, channel));
  if(thread)
    thread->interrupt();
}

void DataService::executeActions()
{
  for(;;)
  {
    Action* action = 0;
    if(!actionQueue.get(action, 0) || !action)
      break;
    switch(action->type)
    {
    case Action::Type::addTrade:
      {
        AddTradeAction* addTradeAction = (AddTradeAction*)action;
        PublicDataModel* publicDataModel = activeSubscriptions[addTradeAction->channelId];
        if(publicDataModel)
        {
          DataProtocol::Trade& trade = addTradeAction->trade;
          bool isSyncOrLive = addTradeAction->trade.flags & DataProtocol::syncFlag || !(addTradeAction->trade.flags & DataProtocol::replayedFlag);
          publicDataModel->addTrade(trade.id, trade.time / 1000ULL, trade.price, trade.amount, isSyncOrLive);
        }
      }
      break;
    case Action::Type::logMessage:
      {
        LogMessageAction* logMessageAction = (LogMessageAction*)action;
        dataModel.logModel.addMessage(logMessageAction->type, logMessageAction->message);
      }
      break;
    case Action::Type::setState:
      {
        SetStateAction* setStateAction = (SetStateAction*)action;
        //publicDataModel.setState(setStateAction->state);
        if(setStateAction->state == PublicDataModel::State::connected)
          for(QMap<QString, PublicDataModel*>::ConstIterator i = dataModel.getDataChannels().begin(), end = dataModel.getDataChannels().end(); i != end; ++i)
          {
            PublicDataModel* publicDataModel = i.value();
            if(publicDataModel)
              publicDataModel->setState(PublicDataModel::State::connecting);
          }
        if(setStateAction->state == PublicDataModel::State::offline)
          for(QMap<QString, PublicDataModel*>::ConstIterator i = dataModel.getDataChannels().begin(), end = dataModel.getDataChannels().end(); i != end; ++i)
          {
            PublicDataModel* publicDataModel = i.value();
            if(publicDataModel)
              publicDataModel->setState(PublicDataModel::State::offline);
          }
      }
      break;
    case Action::Type::channelInfo:
      {
        ChannelInfoAction* channelInfo = (ChannelInfoAction*)action;
        dataModel.addDataChannel(channelInfo->channel);
      }
      break;
    case Action::Type::subscribeResponse:
      {
        SubscribeResponseAction* subscribeResponse = (SubscribeResponseAction*)action;
        PublicDataModel& publicDataModel = dataModel.getDataChannel(subscribeResponse->channel);
        activeSubscriptions[subscribeResponse->channelId] = &publicDataModel;
        publicDataModel.setState(PublicDataModel::State::connected);
        dataModel.logModel.addMessage(LogModel::Type::information, QString("Subscribed to channel %1.").arg(subscribeResponse->channel));
      }
      break;
    case Action::Type::unsubscribeResponse:
      {
        SubscribeResponseAction* unsubscribeResponse = (SubscribeResponseAction*)action;
        PublicDataModel& publicDataModel = dataModel.getDataChannel(unsubscribeResponse->channel);
        activeSubscriptions.remove(unsubscribeResponse->channelId);
        if(publicDataModel.getState() == PublicDataModel::State::connected)
        {
          publicDataModel.setState(PublicDataModel::State::offline);
        }
        dataModel.logModel.addMessage(LogModel::Type::information, QString("Unsubscribed from channel %1.").arg(unsubscribeResponse->channel));
      }
      break;
      // todo: unsubscripe response, set channel to offline
    default:
      break;
    }

    delete action;
  }
}
