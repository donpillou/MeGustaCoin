
#include "stdafx.h"

EBotSessionItem::EBotSessionItem(quint32 id, const EBotSessionItemDraft& sessionItem) : Entity(eType, id)
{
  type = sessionItem.getType();
  state = sessionItem.getState();
  date = sessionItem.getDate();
  price = sessionItem.getPrice();
  amount = sessionItem.getAmount();
  profitablePrice = sessionItem.getProfitablePrice();
  flipPrice = sessionItem.getFlipPrice();
  orderId = sessionItem.getOrderId();
}
