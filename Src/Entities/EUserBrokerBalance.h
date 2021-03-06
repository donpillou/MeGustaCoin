
#pragma once

class EUserBrokerBalance : public Entity
{
public:
  static const EType eType = EType::userBrokerBalance;

public:
  EUserBrokerBalance(const meguco_user_broker_balance_entity& data) : Entity(eType, 0)
  {
    reservedUsd = data.reserved_usd;
    reservedBtc = data.reserved_btc;
    availableUsd = data.available_usd;
    availableBtc = data.available_btc;
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
