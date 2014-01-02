
#include "stdafx.h"

MarketStreamService::MarketStreamService(QObject* parent, DataModel& dataModel, PublicDataModel& publicDataModel, const QString& marketName) :
  QObject(parent),
  dataModel(dataModel), publicDataModel(publicDataModel), marketName(marketName), marketStream(0), thread(0), canceled(true)
{
  int features = 0;
  if(marketName == "MtGox/USD")
    features = (int)MarketStream::Features::trades;
  else if(marketName == "Bitstamp/USD")
    features = (int)MarketStream::Features::trades;
  else if(marketName == "BtcChina/CNY")
    features = (int)MarketStream::Features::trades;

  publicDataModel.setMarket(marketName, features);
}

MarketStreamService::~MarketStreamService()
{
  unsubscribe();

  qDeleteAll(actionQueue.getAll());
}

void MarketStreamService::subscribe()
{
  if(thread)
    return;

  if(marketName == "MtGox/USD")
    marketStream = new MtGoxMarketStream();
  else if(marketName == "Bitstamp/USD")
    marketStream = new BitstampMarketStream();
  else if(marketName == "BtcChina/CNY")
    marketStream = new BtcChinaMarketStream();
  if(!marketStream)
    return;

  struct WorkerThread : public QThread, public MarketStream::Callback
  {
    WorkerThread(MarketStreamService& streamService, MarketStream& marketStream, JobQueue<Action*>& actionQueue, const bool& canceled) :
      streamService(streamService), marketStream(marketStream), actionQueue(actionQueue), canceled(canceled) {}

    virtual void run()
    {
      while(!canceled)
      {
        marketStream.process(*this);
        if(canceled)
          return;
        sleep(10);
      }
    }

    virtual void receivedTrade(const MarketStream::Trade& trade)
    {
      actionQueue.append(new AddTradeAction(trade));
      QTimer::singleShot(0, &streamService, SLOT(executeActions()));
    }

    virtual void error(const QString& message)
    {
      actionQueue.append(new LogMessageAction(LogModel::Type::error, message));
      QTimer::singleShot(0, &streamService, SLOT(executeActions()));
    }

    virtual void information(const QString& message)
    {
      actionQueue.append(new LogMessageAction(LogModel::Type::information, message));
      QTimer::singleShot(0, &streamService, SLOT(executeActions()));
    }

    MarketStreamService& streamService;
    MarketStream& marketStream;
    JobQueue<Action*>& actionQueue;
    const bool& canceled;
  };

  canceled = false;
  thread = new WorkerThread(*this, *marketStream, actionQueue, canceled);
  thread->start();

  publicDataModel.setMarket(marketStream->getCoinCurrency(), marketStream->getMarketCurrency());
  //dataModel.logModel.addMessage(LogModel::Type::information,  tr("Started listening to %1").arg(marketName));
}

void MarketStreamService::unsubscribe()
{
  if(!thread)
    return;

  QString marketName = this->marketName;
  marketStream->cancel();
  canceled = true;
  thread->wait();
  delete thread;
  thread = 0;
  delete marketStream;
  marketStream = 0;

  qDeleteAll(actionQueue.getAll());

  //dataModel.logModel.addMessage(LogModel::Type::information,  tr("Stopped listening to %1").arg(marketName));
}

void MarketStreamService::executeActions()
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
        publicDataModel.addTrade(addTradeAction->trade);
      }
      break;
    case Action::Type::logMessage:
      {
        LogMessageAction* logMessageAction = (LogMessageAction*)action;
        dataModel.logModel.addMessage(logMessageAction->type, logMessageAction->message);
      }
      break;
    }

    delete action;
  }
}
