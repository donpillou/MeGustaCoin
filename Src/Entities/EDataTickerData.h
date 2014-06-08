
#pragma once

class EDataTickerData : public Entity
{
public:
  static const EType eType = EType::dataTickerData;

public:
  EDataTickerData(const DataProtocol::Ticker& ticker) : Entity(eType, 0) {setData(ticker);}

  double getAsk() const {return ask;}
  double getBid() const {return bid;}

  void setData(const DataProtocol::Ticker& ticker)
  {
    this->bid = ticker.bid;
    this->ask = ticker.ask;
  }

private:
  double ask;
  double bid;
};
