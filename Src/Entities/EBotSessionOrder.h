
#pragma once

class EBotSessionOrder : public Entity
{
public:
  static const EType eType = EType::botSessionOrder;

public:
  enum class Type
  {
    buy = BotProtocol::Order::buy,
    sell = BotProtocol::Order::sell,
  };

public:
  EBotSessionOrder(BotProtocol::Order& data) : Entity(eType, data.entityId)
  {
    type = (Type)data.type;
    date = QDateTime::fromMSecsSinceEpoch(data.date);
    price = data.price;
    amount = data.amount;
    total = data.total;
    fee = qAbs(total - price * amount);
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
  double price;
  double amount;
  double fee;
  double total;
};
