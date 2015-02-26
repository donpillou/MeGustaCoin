
#pragma once

class EDataTradeData : public Entity
{
public:
  static const EType eType = EType::dataTradeData;

public:
  struct Trade
  {
    quint64 id;
    quint64 time;
    double price;
    double amount;
  };

public:
  EDataTradeData() : Entity(eType, 0) {}
  EDataTradeData(const Trade& trade) : Entity(eType, 0) {data.append(trade);}

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
