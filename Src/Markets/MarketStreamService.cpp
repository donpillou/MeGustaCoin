
#include "stdafx.h"

MarketStreamService::MarketStreamService(QObject* parent, DataModel& dataModel, PublicDataModel& publicDataModel, const QString& marketName) :
  QObject(parent),
  dataModel(dataModel), publicDataModel(publicDataModel), marketName(marketName), marketStream(0), thread(0)
{
  int features = 0;
  if(marketName == "MtGox/USD")
    features = (int)MarketStream::Features::trades;
  else if(marketName == "Bitstamp/USD")
    features = (int)MarketStream::Features::trades;

  publicDataModel.setMarket(marketName, features);
}

MarketStreamService::~MarketStreamService()
{
  unsubscribe();
}

void MarketStreamService::subscribe()
{
  if(thread)
    return;

  if(marketName == "MtGox/USD")
    marketStream = new MtGoxMarketStream();
  else if(marketName == "Bitstamp/USD")
    marketStream = new BitstampMarketStream();
  if(!marketStream)
    return;

  struct WorkerThread : public QThread, public MarketStream::Callback
  {
    WorkerThread(MarketStreamService& service, MarketStream& marketStream, DataModel& dataModel, PublicDataModel& publicDataModel) :
      service(service), marketStream(marketStream), dataModel(dataModel), publicDataModel(publicDataModel) {}

    virtual void run()
    {
      marketStream.loop(*this);
    }

    virtual void receivedTrade(const MarketStream::Trade& trade)
    {
      publicDataModel.addTrade(trade);
    }

    virtual void error(const QString& message)
    {
      dataModel.logModel.addMessage(LogModel::Type::error, message);
    }

    MarketStreamService& service;
    MarketStream& marketStream;
    DataModel& dataModel;
    PublicDataModel& publicDataModel;
  };

  thread = new WorkerThread(*this, *marketStream, dataModel, publicDataModel);
  thread->start();

  publicDataModel.setMarket(marketStream->getCoinCurrency(), marketStream->getMarketCurrency());
  dataModel.logModel.addMessage(LogModel::Type::information,  tr("Started listening to %1").arg(marketName));
}

void MarketStreamService::unsubscribe()
{
  if(!thread)
    return;

  QString marketName = this->marketName;
  marketStream->cancel();
  thread->wait();
  delete thread;
  thread = 0;
  delete marketStream;
  marketStream = 0;

  dataModel.logModel.addMessage(LogModel::Type::information,  tr("Stopped listening to %1").arg(marketName));
}
