
#include "stdafx.h"

EUserBrokerOrder::EUserBrokerOrder(quint64 id, const EUserBrokerOrderDraft& order) : Entity(eType, id)
{
  type = order.getType();
  date = order.getDate();
  price = order.getPrice();
  amount = order.getAmount();
  fee = order.getFee();
  total = order.getTotal();
  state = State::open;
}
