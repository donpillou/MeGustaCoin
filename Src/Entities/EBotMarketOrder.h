
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
  EBotMarketOrder(EType type, quint32 id) : Entity(type, id) {}
  EBotMarketOrder(BotProtocol::Order& data) : Entity(eType, data.entityId)
  {
    type = (Type)data.type;
    date = QDateTime::fromMSecsSinceEpoch(data.date);
    price = data.price;
    amount = data.amount;
    fee = data.fee;

    updateTotal();
    state = State::open;
  }

  Type getType() const {return type;}
  const QDateTime& getDate() const {return date;}
  double getPrice() const {return price;}
  void setPrice(double price)
  {
    this->price = price;
    updateTotal();
  }
  double getAmount() const {return amount;}
  void setAmount(double amount)
  {
    this->amount = amount;
    updateTotal();
  }
  double getFee() const {return fee;}
  void setFee(double fee)
  {
    this->fee = fee;
    updateTotal();
  }
  double getTotal() const {return total;}
  State getState() const {return state;}
  void setState(State state) {this->state = state;}

protected:
  Type type;
  QDateTime date;
  double price;
  double amount;
  double fee;
  double total;
  State state;

  void updateTotal()
  {
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
};
