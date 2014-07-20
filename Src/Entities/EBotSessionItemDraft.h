
#pragma once

class EBotSessionItemDraft : public EBotSessionItem
{
public:
  static const EType eType = EType::botSessionItemDraft;

public:
  EBotSessionItemDraft(quint32 id, Type type, const QDateTime& date, double flipPrice) : EBotSessionItem(eType, id)
  {
    this->type = type;
    this->state = State::draft;
    this->date = date;
    this->price = 0.;
    this->balanceComm = 0.;
    this->balanceBase = 0.;
    this->profitablePrice = 0.;
    this->flipPrice = flipPrice;
    this->orderId = 0;
  }
};
