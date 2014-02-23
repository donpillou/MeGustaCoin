
#include "stdafx.h"

PublicDataModel::PublicDataModel() :
  tradeModel(*this)/*, bookModel(*this)*/, state(State::offline) {}

void PublicDataModel::setMarket(const QString& marketName)
{
  this->marketName = marketName;
  startTime = QDateTime::currentDateTime().toTime_t();
}

void PublicDataModel::setMarket(const QString& coinCurrency, const QString& marketCurrency)
{
  this->coinCurrency = coinCurrency;
  this->marketCurrency = marketCurrency;
  emit changedMarket();
}

QString PublicDataModel::formatAmount(double amount) const
{
  return QLocale::system().toString(fabs(amount), 'f', 8);
}

QString PublicDataModel::formatPrice(double price) const
{
  return QLocale::system().toString(price, 'f', 2);
}

void PublicDataModel::addTrade(quint64 id, quint64 time, double price, double amount, bool isSyncOrLive)
{
  if((qint64)startTime - (qint64)time < 30 * 60)
    tradeModel.addTrade(id, time, price, amount);
  graphModel.addTrade(id, time, price, amount, isSyncOrLive);
}

//void PublicDataModel::addTickerData(const MarketStream::TickerData& tickerData)
//{
//  graphModel.addTickerData(tickerData);
//  emit updatedTicker();
//}

//void PublicDataModel::setBookData(quint64 time, const QList<MarketStream::OrderBookEntry>& askItems, const QList<MarketStream::OrderBookEntry>& bidItems)
//{
//  if(time == bookModel.getTime())
//    return;
//
//  bookModel.setData(time, askItems, bidItems);
//
//  enum SumType
//  {
//    sum50,
//    sum100,
//    sum250,
//    sum500,
//    numOfSumType
//  };
//  double sumMax[numOfSumType] = {50., 100., 250., 500.};
//
//  double askSum[numOfSumType] = {};  
//  double askSumMass[numOfSumType] = {};
//  for(int i = askItems.size() - 1; i >= 0; --i)
//  {
//    const MarketStream::OrderBookEntry& item = askItems[i];
//    for(int i = 0; i < numOfSumType; ++i)
//      if(askSum[i] < sumMax[i])
//      {
//        double amount = qMin(item.amount, sumMax[i] - askSum[i]);
//        askSumMass[i] += item.price * amount;
//        askSum[i] += amount;
//      }
//    if(askSum[numOfSumType - 1] >= sumMax[numOfSumType - 1])
//      break;
//  }
//
//  double bidSum[numOfSumType] = {};  
//  double bidSumMass[numOfSumType] = {};
//  for(int i = bidItems.size() - 1; i >= 0; --i)
//  {
//    const MarketStream::OrderBookEntry& item = bidItems[i];
//    for(int i = 0; i < numOfSumType; ++i)
//      if(bidSum[i] < sumMax[i])
//      {
//        double amount = qMin(item.amount, sumMax[i] - bidSum[i]);
//        bidSumMass[i] += item.price * amount;
//        bidSum[i] += amount;
//      }
//    if(bidSum[numOfSumType - 1] >= sumMax[numOfSumType - 1])
//      break;
//  }
//
//  GraphModel::BookSample summary;
//  summary.time = time;
//  summary.ask = askItems.isEmpty() ? 0 : askItems.back().price;
//  summary.bid = bidItems.isEmpty() ? 0 : bidItems.back().price;
//  summary.comPrice[(int)GraphModel::BookSample::ComPrice::comPrice100] = askSum[sum50] >= sumMax[sum50] && bidSum[sum50] >= sumMax[sum50] ? (askSumMass[sum50] + bidSumMass[sum50]) / (askSum[sum50] + bidSum[sum50]) : 0;
//  summary.comPrice[(int)GraphModel::BookSample::ComPrice::comPrice200] = askSum[sum100] >= sumMax[sum100] && bidSum[sum100] >= sumMax[sum100] ? (askSumMass[sum100] + bidSumMass[sum100]) / (askSum[sum100] + bidSum[sum100]) : summary.comPrice[(int)GraphModel::BookSample::ComPrice::comPrice100];
//  summary.comPrice[(int)GraphModel::BookSample::ComPrice::comPrice500] = askSum[sum250] >= sumMax[sum250] && bidSum[sum250] >= sumMax[sum250] ? (askSumMass[sum250] + bidSumMass[sum250]) / (askSum[sum250] + bidSum[sum250]) : summary.comPrice[(int)GraphModel::BookSample::ComPrice::comPrice200];
//  summary.comPrice[(int)GraphModel::BookSample::ComPrice::comPrice1000] = askSum[sum500] >= sumMax[sum500] && bidSum[sum500] >= sumMax[sum500] ? (askSumMass[sum500] + bidSumMass[sum500]) / (askSum[sum500] + bidSum[sum500]) : summary.comPrice[(int)GraphModel::BookSample::ComPrice::comPrice500];
//  graphModel.addBookSample(summary);
//}

void PublicDataModel::setState(State state)
{
  if(this->state == state)
    return;
  this->state = state;
  emit changedState();
}

QString PublicDataModel::getStateName() const
{
  switch(state)
  {
  case PublicDataModel::State::connecting:
    return tr("connecting...");
  case PublicDataModel::State::offline:
    return tr("offline");
  case PublicDataModel::State::connected:
    return QString();
  }
  Q_ASSERT(false);
  return QString();
}
