
#pragma once

class EOrder : public Entity
{
public:
  static const EType eType = EType::order;

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
  //EOrder(quint32 id) : Entity(eType, id), type(Type::unknown), amount(0.), price(0.), total(0.), state(State::open) {}
  EOrder(quint32 id, BotProtocol::Order& data) : Entity(eType, id)
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
