
#pragma once

class EBotMarketOrderDraft;

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
    updating,
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
    total = data.total;
    fee = qAbs(total - price * amount);
    state = State::open;
  }
  EBotMarketOrder(quint32 id, const EBotMarketOrderDraft& order);

  Type getType() const {return type;}
  const QDateTime& getDate() const {return date;}
  double getPrice() const {return price;}
  void setPrice(double price, double total)
  {
    this->price = price;
    this->total = total;
    fee = qAbs(total - price * amount);
  }
  double getAmount() const {return amount;}
  void setAmount(double amount, double total)
  {
    this->amount = amount;
    this->total = total;
    fee = qAbs(total - price * amount);
  }
  double getFee() const {return fee;}
  double getTotal() const {return total;}
  State getState() const {return state;}
  void setState(State state) {this->state = state;}

protected:
  EBotMarketOrder(EType type, quint32 id) : Entity(type, id) {}

protected:
  Type type;
  QDateTime date;
  double price;
  double amount;
  double fee;
  double total;
  State state;
};
