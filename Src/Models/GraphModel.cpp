
#include "stdafx.h"

void GraphModel::addTrade(const MarketStream::Trade& trade)
{
  TradeSample* tradeSample;
  if(tradeSamples.isEmpty() || tradeSamples.last().time != trade.date)
    tradeSamples.append(TradeSample());
  tradeSample =  &tradeSamples.last();

  tradeSample->time = trade.date;
  tradeSample->last = trade.price;
  if(tradeSample->amount == 0.)
    tradeSample->min = tradeSample->max = tradeSample->first = trade.price;
  else if(trade.price < tradeSample->min)
    tradeSample->min = trade.price;
  else if(trade.price > tradeSample->max)
    tradeSample->max = trade.price;
  tradeSample->amount += trade.amount;

  double depths[] = {10., 20., 50., 100., 200., 500., 1000.};
  for (int i = 0; i < (int)RegressionDepth::numOfRegressionDepths; ++i)
  {
    averager[i].add(trade.date, trade.amount, trade.price);
    if(i == (int)RegressionDepth::depth24h)
      averager[i].limitToAge(24 * 60 * 60);
    else
      averager[i].limitToVolume(depths[i]);
    averager[i].getLine(regressionLines[i].a, regressionLines[i].b, regressionLines[i].startTime, regressionLines[i].endTime);
    regressionLines[i].averagePrice = averager[i].getAveragePrice();
  }

  while(!tradeSamples.isEmpty() && trade.date - tradeSamples.front().time > 7 * 24 * 60 * 60)
    tradeSamples.pop_front();

  emit dataAdded();
}

void GraphModel::addBookSample(const BookSample& bookSample)
{
  bookSamples.append(bookSample);

  quint64 time = bookSample.time;
  while(!bookSamples.isEmpty() && time - bookSamples.front().time > 7 * 24 * 60 * 60)
    bookSamples.pop_front();

  emit dataAdded();
}

void GraphModel::addTickerData(const MarketStream::TickerData& tickerData)
{
  tickerSamples.append(tickerData);

  emit dataAdded();
}
