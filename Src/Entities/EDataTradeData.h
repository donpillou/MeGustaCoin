
#pragma once

class EDataTradeData : public Entity
{
public:
  static const EType eType = EType::dataTradeData;

public:
  EDataTradeData() : Entity(eType, 0) {}
  EDataTradeData(const DataProtocol::Trade& trade) : Entity(eType, 0) {data.append(trade);}

  const QList<DataProtocol::Trade>& getData() const {return data;}
  void addTrade(const DataProtocol::Trade& trade) {data.append(trade);}

  void setData(const DataProtocol::Trade& trade)
  {
    if(data.size() != 1)
    {
      QList<DataProtocol::Trade> data;
      data.append(trade);
      this->data.swap(data);
    }
    else
      data.front() = trade;
  }

private:
  QList<DataProtocol::Trade> data;
};
