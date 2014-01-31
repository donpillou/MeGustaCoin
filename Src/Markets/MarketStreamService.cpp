
#include "stdafx.h"

MarketStreamService::MarketStreamService(QObject* parent, DataModel& dataModel, PublicDataModel& publicDataModel, const QString& marketName) :
  QObject(parent),
  dataModel(dataModel), publicDataModel(publicDataModel), marketName(marketName), marketStream(0), thread(0), canceled(true)
{
  publicDataModel.setMarket(marketName);
}

MarketStreamService::~MarketStreamService()
{
  unsubscribe();

  qDeleteAll(actionQueue.getAll());
}

void MarketStreamService::subscribe()
{
  return;
  if(thread)
    return;

  if(marketName == "MtGox/USD")
    marketStream = new MtGoxMarketStream();
  else if(marketName == "Bitstamp/USD")
    marketStream = new BitstampMarketStream();
  else if(marketName == "BtcChina/CNY")
    marketStream = new BtcChinaMarketStream();
  else if(marketName == "Huobi/CNY")
    marketStream = new HuobiMarketStream();
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
        actionQueue.append(new SetStateAction(PublicDataModel::State::connecting));
        QTimer::singleShot(0, &streamService, SLOT(executeActions()));
        marketStream.process(*this);
        actionQueue.append(new SetStateAction(PublicDataModel::State::offline));
        QTimer::singleShot(0, &streamService, SLOT(executeActions()));
        if(canceled)
          return;
        sleep(10);
      }
    }

    virtual void connected()
    {
      actionQueue.append(new SetStateAction(PublicDataModel::State::connected));
      QTimer::singleShot(0, &streamService, SLOT(executeActions()));
    }

    virtual void receivedTrade(const MarketStream::Trade& trade)
    {
      actionQueue.append(new AddTradeAction(trade));
      QTimer::singleShot(0, &streamService, SLOT(executeActions()));
    }

    virtual void receivedTickerData(const MarketStream::TickerData& tickerData)
    {
      actionQueue.append(new AddTickerDataAction(tickerData));
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
  canceled = true;
  marketStream->cancel();
  thread->wait();
  delete thread;
  thread = 0;
  delete marketStream;
  marketStream = 0;

  executeActions(); // better than qDeleteAll(actionQueue.getAll()); ;)

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
    case Action::Type::addTickerData:
      {
        AddTickerDataAction* addTickerDataAction = (AddTickerDataAction*)action;
        publicDataModel.addTickerData(addTickerDataAction->tickerData);
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
        publicDataModel.setState(setStateAction->state);
      }
      break;
    }

    delete action;
  }
}
