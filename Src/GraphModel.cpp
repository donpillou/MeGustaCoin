
#include "stdafx.h"

void GraphModel::addTrade(quint64 time, double price, double amount)
{
  TradeSample* tradeSample;
  if(tradeSamples.isEmpty() || tradeSamples.last().time != time)
    tradeSamples.append(TradeSample());
  tradeSample =  &tradeSamples.last();

  tradeSample->time = time;
  tradeSample->last = price;
  tradeSample->amount += amount;
  if(tradeSample->min == 0.)
    tradeSample->min = tradeSample->max = price;
  else if(price < tradeSample->min)
    tradeSample->min = price;
  else if(price > tradeSample->max)
    tradeSample->max = price;

  double depths[] = {10., 20., 50., 100., 200., 500., 1000.};
  for (int i = 0; i < (int)RegressionDepth::numOfRegressionDepths; ++i)
  {
    averager[i].add(time, amount, price);
    averager[i].limitTo(depths[i]);
    averager[i].getLine(regressionLines[i].a, regressionLines[i].b, regressionLines[i].startTime, regressionLines[i].endTime);
  }

  while(!tradeSamples.isEmpty() && time - tradeSamples.front().time > 7 * 24 * 60 * 60)
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
