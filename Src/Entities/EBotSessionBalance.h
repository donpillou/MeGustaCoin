
#pragma once

class EBotSessionBalance : public Entity
{
public:
  static const EType eType = EType::botSessionBalance;

public:
  EBotSessionBalance(BotProtocol::Balance& data) : Entity(eType, data.entityId)
  {
    reservedUsd = data.reservedUsd;
    reservedBtc = data.reservedBtc;
    availableUsd = data.availableUsd;
    availableBtc = data.availableBtc;
    fee = data.fee;
  }

  double getReservedUsd() const {return reservedUsd;}
  double getReservedBtc() const {return reservedBtc;}
  double getAvailableUsd() const {return availableUsd;}
  double getAvailableBtc() const {return availableBtc;}

  double getFee() const {return fee;}
  
private:
  double reservedUsd;
  double reservedBtc;
  double availableUsd;
  double availableBtc;
  double fee;
};
