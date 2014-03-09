
#include "stdafx.h"

GraphModel::GraphModel() : synced(false), vwap24(0.), values(0)
{
//  for(int i = 0; i < sizeof(estimations) / sizeof(*estimations); ++i)
//  {
//    Estimator& estimator = estimations[i];
//    estimator.tradeVariance = (i + 1) * 2. * (i + 1) * 2.;
//    estimator.estimateVariance = 5 * 5;
//  }
}

void GraphModel::addTrade(const DataProtocol::Trade& trade)
{
  quint64 time = trade.time / 1000;

  TradeSample* tradeSample;
  if(tradeSamples.isEmpty() || tradeSamples.last().time != time)
    tradeSamples.append(TradeSample());
  tradeSample =  &tradeSamples.last();

  tradeSample->time = time;
  tradeSample->last = trade.price;
  if(tradeSample->amount == 0.)
    tradeSample->min = tradeSample->max = tradeSample->first = trade.price;
  else if(trade.price < tradeSample->min)
    tradeSample->min = trade.price;
  else if(trade.price > tradeSample->max)
    tradeSample->max = trade.price;
  tradeSample->amount += trade.amount;

  while(!tradeSamples.isEmpty() && time - tradeSamples.front().time > 7 * 24 * 60 * 60)
    tradeSamples.pop_front();

  bool isSyncOrLive = trade.flags & DataProtocol::syncFlag || !(trade.flags & DataProtocol::replayedFlag);
  tradeHander.add(trade, isSyncOrLive);

  if(isSyncOrLive)
  {
    values = &tradeHander.values;
    synced = true;
    emit dataAdded();
  }
}

//void GraphModel::addBookSample(const BookSample& bookSample)
//{
//  bookSamples.append(bookSample);
//
//  quint64 time = bookSample.time;
//  while(!bookSamples.isEmpty() && time - bookSamples.front().time > 7 * 24 * 60 * 60)
//    bookSamples.pop_front();
//
//  emit dataAdded();
//}

//void GraphModel::addTickerData(const MarketStream::TickerData& tickerData)
//{
//  tickerSamples.append(tickerData);
//
//  while(!tickerSamples.isEmpty() && tickerData.date - tickerSamples.front().date > 7 * 24 * 60 * 60)
//    tickerSamples.pop_front();
//
//  emit dataAdded();
//}
