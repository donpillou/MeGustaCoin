
#pragma once

class EMarketTickerData : public Entity
{
public:
  static const EType eType = EType::marketTickerData;

public:
  EMarketTickerData(double ask, double bid) : Entity(eType, 0), ask(ask), bid(bid) {}

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
