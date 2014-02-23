
#include "stdafx.h"

GraphModel::GraphModel() : synced(false)
{
//  for(int i = 0; i < sizeof(estimations) / sizeof(*estimations); ++i)
//  {
//    Estimator& estimator = estimations[i];
//    estimator.tradeVariance = (i + 1) * 2. * (i + 1) * 2.;
//    estimator.estimateVariance = 5 * 5;
//  }
}

void GraphModel::addTrade(quint64 id, quint64 time, double price, double amount, bool isSyncOrLive)
{
  TradeSample* tradeSample;
  if(tradeSamples.isEmpty() || tradeSamples.last().time != time)
    tradeSamples.append(TradeSample());
  tradeSample =  &tradeSamples.last();

  tradeSample->time = time;
  tradeSample->last = price;
  if(tradeSample->amount == 0.)
    tradeSample->min = tradeSample->max = tradeSample->first = price;
  else if(price < tradeSample->min)
    tradeSample->min = price;
  else if(price > tradeSample->max)
    tradeSample->max = price;
  tradeSample->amount += amount;

  /*
      depth1m,
    depth3m,
    depth5m,
    depth10m,
    depth15m,
    depth20m,
    depth30m,
    depth1h,
    depth2h,
    depth4h,
    depth6h,
    depth12h,
    depth24h,
    */

  quint64 depths[] = {1 * 60, 3 * 60, 5 * 60, 10 * 60, 15 * 60, 20 * 60, 30 * 60, 1 * 60 * 60, 2 * 60 * 60, 4 * 60 * 60, 6 * 60 * 60, 12 * 60 * 60, 24 * 60 * 60};
  for(int i = 0; i < (int)RegressionDepth::numOfRegressionDepths; ++i)
  {
    averager[i].add(time, amount, price);
    averager[i].limitToAge(depths[i]);
    if(isSyncOrLive)
    {
      averager[i].getLine(regressionLines[i].a, regressionLines[i].b, regressionLines[i].startTime, regressionLines[i].endTime);
      regressionLines[i].averagePrice = averager[i].getAveragePrice();
    }
  }

  for(int i = 0; i < (int)RegressionDepth::numOfExpRegessionDepths; ++i)
  {
    expAverager[i].add(time, amount, price, depths[i]);
    if(isSyncOrLive)
    {
      expAverager[i].getLine(depths[i], expRegressionLines[i].a, expRegressionLines[i].b, expRegressionLines[i].startTime, expRegressionLines[i].endTime);
      expRegressionLines[i].averagePrice = expAverager[i].getAveragePrice();
    }
  }

//  for(int i = 0; i < sizeof(estimations) / sizeof(*estimations); ++i)
//  {
//    Estimator& estimator = estimations[i];
//    estimator.add(time, amount, price);
//  }

  while(!tradeSamples.isEmpty() && time - tradeSamples.front().time > 7 * 24 * 60 * 60)
    tradeSamples.pop_front();

  if(isSyncOrLive)
  {
    synced = true;
    emit dataAdded();
  }
}

void GraphModel::addBookSample(const BookSample& bookSample)
{
  bookSamples.append(bookSample);

  quint64 time = bookSample.time;
  while(!bookSamples.isEmpty() && time - bookSamples.front().time > 7 * 24 * 60 * 60)
    bookSamples.pop_front();

  emit dataAdded();
}

//void GraphModel::addTickerData(const MarketStream::TickerData& tickerData)
//{
//  tickerSamples.append(tickerData);
//
//  while(!tickerSamples.isEmpty() && tickerData.date - tickerSamples.front().date > 7 * 24 * 60 * 60)
//    tickerSamples.pop_front();
//
//  emit dataAdded();
//}
