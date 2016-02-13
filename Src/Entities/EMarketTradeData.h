
#pragma once

class EMarketTradeData : public Entity
{
public:
  static const EType eType = EType::marketTradeData;

public:
  struct Trade
  {
    quint64 id;
    quint64 time;
    double price;
    double amount;
  };

public:
  EMarketTradeData() : Entity(eType, 0) {}
  EMarketTradeData(const Trade& trade) : Entity(eType, 0) {data.append(trade);}

  const QList<Trade>& getData() const {return data;}
  void addTrade(const Trade& trade) {data.append(trade);}

  void setData(const Trade& trade)
  {
    if(data.size() != 1)
    {
      QList<Trade> data;
      data.append(trade);
      this->data.swap(data);
    }
    else
      data.front() = trade;
  }

private:
  QList<Trade> data;
};
