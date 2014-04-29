
#pragma once

class EBotMarketOrder : public Entity
{
public:
  static const EType eType = EType::botMarketOrder;

public:
  enum class Type
  {
    buy = BotProtocol::Order::buy,
    sell = BotProtocol::Order::sell,
  };

  enum class State
  {
    draft,
    submitting,
    open,
    canceling,
    canceled,
    closed,
  };

public:
  EBotMarketOrder(BotProtocol::Order& data) : Entity(eType, data.entityId)
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

    state = State::open;
  }

  Type getType() const {return type;}
  const QDateTime& getDate() const {return date;}
  double getPrice() const {return price;}
  double getAmount() const {return amount;}
  double getFee() const {return fee;}
  double getTotal() const {return total;}
  State getState() const {return state;}

private:
  Type type;
  QDateTime date;
  double price;
  double amount;
  double fee;
  double total;
  State state;
};
