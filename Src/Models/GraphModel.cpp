
#include "stdafx.h"

GraphModel::GraphModel(Entity::Manager& channelEntityManager) : channelEntityManager(channelEntityManager), values(0)
{
  channelEntityManager.registerListener<EDataTradeData>(*this);

  tradeSamples.reserve(7 * 24 * 60 * 60 + 1000);
}

GraphModel::~GraphModel()
{
  channelEntityManager.unregisterListener<EDataTradeData>(*this);
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


void GraphModel::addedEntity(Entity& entity)
{
  EDataTradeData* eDataTradeData = dynamic_cast<EDataTradeData*>(&entity);
  if(eDataTradeData)
  {
    const QList<DataProtocol::Trade>& data = eDataTradeData->getData();
    if(!data.isEmpty())
    {
      qint64 now = data.back().time;
      for(QList<DataProtocol::Trade>::ConstIterator i = data.begin(), end = data.end(); i != end; ++i)
        addTrade(*i, now - i->time);
    }
    return;
  }
  Q_ASSERT(false);
}

void GraphModel::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
{
  addedEntity(newEntity);
}
