
#pragma once

class EUserSessionAssetDraft : public EUserSessionAsset
{
public:
  static const EType eType = EType::userSessionItemDraft;

public:
  EUserSessionAssetDraft(quint64 id, Type type, const QDateTime& date, double flipPrice) : EUserSessionAsset(eType, id)
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
