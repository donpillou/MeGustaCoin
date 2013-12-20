
#include "stdafx.h"

void GraphModel::addTrade(quint64 time, double price)
{
  if(totalMin == 0.)
    totalMin = totalMax = price;
  else if(price < totalMin)
    totalMin = price;
  else if(price > totalMax)
    totalMax = price;

  Entry& entry = trades[time];
  entry.time = time;
  if(entry.min == 0.)
    entry.min = entry.max = price;
  else if(price < entry.min)
    entry.min = price;
  else if(price > entry.max)
    entry.max = price;

  emit dataAdded();
}
