
#include "stdafx.h"

EBotMarketOrder::EBotMarketOrder(quint64 id, const EBotMarketOrderDraft& order) : Entity(eType, id)
{
  type = order.getType();
  date = order.getDate();
  price = order.getPrice();
  amount = order.getAmount();
  fee = order.getFee();
  total = order.getTotal();
  state = State::open;
}
