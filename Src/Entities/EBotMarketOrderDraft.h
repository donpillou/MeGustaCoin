
#pragma once

class EBotMarketOrderDraft : public EBotMarketOrder
{
public:
  static const EType eType = EType::botMarketOrderDraft;

public:
  EBotMarketOrderDraft(quint64 id, Type type, const QDateTime& date, double price) : EBotMarketOrder(eType, id)
  {
    this->type = type;
    this->date = date;
    this->price = price;
    this->amount = 0.;
    this->fee = 0.;
    this->total = 0.;
    this->state = State::draft;
  }
  EBotMarketOrderDraft(quint64 id, const EBotMarketOrder& order) : EBotMarketOrder(eType, id)
  {
    type = order.getType();
    date = order.getDate();
    price = order.getPrice();
    amount = order.getAmount();
    fee = order.getFee();
    total = order.getTotal();
    state = order.getState() == State::canceling ? State::canceled : State::closed;
  }
};
