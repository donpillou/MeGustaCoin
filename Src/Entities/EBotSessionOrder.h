
#pragma once

class EBotSessionOrder : public Entity
{
public:
  static const EType eType = EType::botSessionOrder;

public:
  enum class Type
  {
    buy = meguco_user_market_order_buy,
    sell = meguco_user_market_order_sell,
  };

public:
  EBotSessionOrder(meguco_user_market_order_entity& data) : Entity(eType, data.entity.id)
  {
    type = (Type)data.type;
    date = QDateTime::fromMSecsSinceEpoch(data.entity.time);
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
