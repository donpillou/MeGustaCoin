
#include "stdafx.h"

EBotSessionItem::EBotSessionItem(quint64 id, const EBotSessionItemDraft& sessionItem) : Entity(eType, id)
{
  type = sessionItem.getType();
  state = sessionItem.getState();
  date = sessionItem.getDate();
  price = sessionItem.getPrice();
  investComm = sessionItem.getInvestComm();
  investBase = sessionItem.getInvestBase();
  balanceComm = sessionItem.getBalanceComm();
  balanceBase = sessionItem.getBalanceBase();
  profitablePrice = sessionItem.getProfitablePrice();
  flipPrice = sessionItem.getFlipPrice();
  orderId = sessionItem.getOrderId();
}
