
#include "stdafx.h"

BotService::BotService() : thread(0) {}

BotService::~BotService()
{
  stop();

  //qDeleteAll(actionQueue.getAll());
}

void BotService::start()
{
  if(thread)
    return;

  struct WorkerThread : public QThread, public DataConnection::Callback
  {
    WorkerThread(JobQueue<Action*>& actionQueue) : actionQueue(actionQueue), canceled(false) {}

    virtual void run()
    {
      while(!canceled)
      {
        //actionQueue.append(new SetStateAction(PublicDataModel::State::connecting));
        //QTimer::singleShot(0, &dataService, SLOT(executeActions()));
        //process();
        //actionQueue.append(new SetStateAction(PublicDataModel::State::offline));
        //QTimer::singleShot(0, &dataService, SLOT(executeActions()));
        //if(canceled)
        //  return;
        sleep(10);
      }
    }

    virtual void receivedChannelInfo(const QString& channelName)
    {
      //actionQueue.append(new ChannelInfoAction(channelName));
      //QTimer::singleShot(0, &dataService, SLOT(executeActions()));
      //information(QString("Found channel %1.").arg(channelName));
    }

    virtual void receivedSubscribeResponse(const QString& channelName, quint64 channelId)
    {
      //actionQueue.append(new SubscribeResponseAction(Action::Type::subscribeResponse, channelName, channelId));
      //QTimer::singleShot(0, &dataService, SLOT(executeActions()));
    }

    virtual void receivedUnsubscribeResponse(const QString& channelName, quint64 channelId)
    {
      //actionQueue.append(new SubscribeResponseAction(Action::Type::unsubscribeResponse, channelName, channelId));
      //QTimer::singleShot(0, &dataService, SLOT(executeActions()));
    }

    virtual void receivedTrade(quint64 channelId, const DataProtocol::Trade& trade)
    {
      //actionQueue.append(new AddTradeAction(channelId, trade));
      //if(trade.flags & DataProtocol::syncFlag || !(trade.flags & DataProtocol::replayedFlag))
      //  QTimer::singleShot(0, &dataService, SLOT(executeActions()));
    }

    virtual void receivedTicker(quint64 channelId, const DataProtocol::Ticker& ticker)
    {
      //actionQueue.append(new AddTickerAction(channelId, ticker));
      //QTimer::singleShot(0, &dataService, SLOT(executeActions()));
    }

    virtual void receivedErrorResponse(const QString& message)
    {
      //actionQueue.append(new LogMessageAction(LogModel::Type::error, message));
      //QTimer::singleShot(0, &dataService, SLOT(executeActions()));
    }

    virtual void information(const QString& message)
    {
      //actionQueue.append(new LogMessageAction(LogModel::Type::information, message));
      //QTimer::singleShot(0, &dataService, SLOT(executeActions()));
    }

    void connected()
    {
      //actionQueue.append(new SetStateAction(PublicDataModel::State::connected));
      //QTimer::singleShot(0, &dataService, SLOT(executeActions()));
    }

    //void process()
    //{
    //  Action* action;
    //  while(subscriptionQueue.get(action, 0))
    //  {
    //    if(!action)
    //    {
    //      canceled = true;
    //      return;
    //    }
    //    delete action;
    //  }
    //
    //  information("Connecting to data service...");
    //
    //  // create connection
    //  if(!connection.connect())
    //  {
    //    receivedErrorResponse(QString("Could not connect to data service: %1").arg(connection.getLastError()));
    //    return;
    //  }
    //  information("Connected to data service.");
    //  connected();
    //
    //  // load channel list
    //  if(!connection.loadChannelList())
    //    goto error;
    //
    //  // loop
    //  for(;;)
    //  {
    //    while(subscriptionQueue.get(action, 0))
    //    {
    //      if(!action)
    //      {
    //        canceled = true;
    //        return;
    //      }
    //      switch(action->type)
    //      {
    //      case Action::Type::subscribe:
    //        {
    //          SubscriptionAction* subscriptionAction = (SubscriptionAction*)action;
    //          if(!connection.subscribe(subscriptionAction->channel, subscriptionAction->lastReceivedTradeId))
    //            goto error;
    //        }
    //        break;
    //      case Action::Type::unsubscribe:
    //        {
    //          SubscriptionAction* subscriptionAction = (SubscriptionAction*)action;
    //          if(!connection.unsubscribe(subscriptionAction->channel))
    //            goto error;
    //        }
    //        break;
    //      default:
    //        break;
    //      }
    //      delete action;
    //    }
    //
    //    if(!connection.process(*this))
    //      break;
    //  }
    //
    //error:
    //  receivedErrorResponse(QString("Lost connection to data service: %1").arg(connection.getLastError()));
    //}

    //DataService& dataService;
    JobQueue<Action*>& actionQueue;
    //JobQueue<Action*>& subscriptionQueue;
    //DataConnection connection;
    bool canceled;
  };

  thread = new WorkerThread(actionQueue);
  thread->start();
}

void BotService::stop()
{
  if(!thread)
    return;

  //subscriptionQueue.append(0); // cancel thread message
  //thread->interrupt();
  thread->wait();
  delete thread;
  thread = 0;

  //executeActions(); // better than qDeleteAll(actionQueue.getAll()); ;)
  //qDeleteAll(subscriptionQueue.getAll());
}


void BotService::executeActions()
{
  for(;;)
  {
    Action* action = 0;
    if(!actionQueue.get(action, 0) || !action)
      break;
    action->execute(*this);
    delete action;
  }
}
