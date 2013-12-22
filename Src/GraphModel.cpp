
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

  while(!tradeSamples.isEmpty() && time - tradeSamples.front().time > 7 * 24 * 60 * 60)
    tradeSamples.pop_front();

  emit dataAdded();
}

void GraphModel::addBookSummary(const BookSummary& bookSummary)
{
  bookSummaries.append(bookSummary);

  quint64 time = bookSummary.time;
  while(!bookSummaries.isEmpty() && time - bookSummaries.front().time > 7 * 24 * 60 * 60)
    bookSummaries.pop_front();

  emit dataAdded();
}
