
#include "stdafx.h"

GraphModel::GraphModel() : values(0)
{
  tradeSamples.reserve(7 * 24 * 60 * 60 + 1000);
}

void GraphModel::addTrade(const DataProtocol::Trade& trade, quint64 tradeAge)
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

  if(tradeAge < 24ULL * 60ULL * 60ULL * 1000ULL)
  {
    tradeHander.add(trade, tradeAge);

    if(tradeAge == 0)
    {
      while(!tradeSamples.isEmpty() && time - tradeSamples.front().time > 7ULL * 24ULL * 60ULL * 60ULL)
        tradeSamples.pop_front();

      values = &tradeHander.values;
      emit dataAdded();
    }
  }
}

void GraphModel::addMarker(quint64 time, Marker marker)
{
  markers.insert(time, marker);
  emit dataAdded();
}

void GraphModel::clearMarkers()
{
  if(markers.isEmpty())
    return;
  markers.clear();
  emit dataAdded();
}
