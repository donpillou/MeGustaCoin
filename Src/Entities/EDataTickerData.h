
#pragma once

class EDataTickerData : public Entity
{
public:
  static const EType eType = EType::dataTickerData;

public:
  EDataTickerData(double ask, double bid) : Entity(eType, 0), ask(ask), bid(bid) {}

  double getAsk() const {return ask;}
  double getBid() const {return bid;}

  void setData(double ask, double bid)
  {
    this->ask = ask;
    this->bid = bid;
  }

private:
  double ask;
  double bid;
};
