
#pragma once

class ETransaction : public Entity
{
public:
  static const EType eType = EType::transaction;

  enum class Type
  {
    buy = BotProtocol::Transaction::buy,
    sell = BotProtocol::Transaction::sell,
  };

public:
  ETransaction(quint32 id, BotProtocol::Transaction& data) : Entity(eType, id)
  {
    type = (Type)data.type;
    date = QDateTime::fromMSecsSinceEpoch(data.date);
    price = data.price;
    amount = data.amount;
    fee = data.fee;

    switch(type)
    {
    case Type::buy:
      total = -(price * amount + fee);
      break;
    case Type::sell:
      total = price * amount - fee;
      break;
    }
  }

  Type getType() const {return type;}
  const QDateTime& getDate() const {return date;}
  double getPrice() const {return price;}
  double getAmount() const {return amount;}
  double getFee() const {return fee;}
  double getTotal() const {return total;}

private:
  Type type;
  QDateTime date;
  double price; // >= 0
  double amount; // >= 0
  double fee; // >= 0
  double total;
};
