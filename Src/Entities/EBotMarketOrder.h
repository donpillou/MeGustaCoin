
#pragma once

class EBotMarketOrderDraft;

class EBotMarketOrder : public Entity // todo: rename to EBotUserBrokerOrder
{
public:
  static const EType eType = EType::botMarketOrder;

public:
  enum class Type
  {
    buy = meguco_user_broker_order_buy,
    sell = meguco_user_broker_order_sell,
  };

  enum class State
  {
    submitting = meguco_user_broker_order_submitting,
    open  = meguco_user_broker_order_open,
    //canceling  = meguco_user_broker_order_canceling,
    canceled  = meguco_user_broker_order_canceled,
    //updating = meguco_user_broker_order_updating,
    closed  = meguco_user_broker_order_closed,
    //removing  = meguco_user_broker_order_removing,
    error = meguco_user_broker_order_error,
    draft,
    canceling,
    updating,
    removing,
  };

public:
  EBotMarketOrder(const meguco_user_broker_order_entity& data) : Entity(eType, data.entity.id)
  {
    type = (Type)data.type;
    date = QDateTime::fromMSecsSinceEpoch(data.entity.time);
    price = data.price;
    amount = data.amount;
    total = data.total;
    fee = qAbs(total - price * amount);
    state = (State)data.state;
    //rawId = data.raw_id;
    //timeout = data.timeout;
  }

  EBotMarketOrder(quint64 id, const EBotMarketOrderDraft& order);
  /*
  void toEntity(meguco_user_market_order_entity& entity) const
  {
    entity.entity.id = id;
    entity.entity.time = date.toMSecsSinceEpoch();
    entity.entity.size = sizeof(entity);
    entity.type = (uint8_t)type;
    entity.state = (uint8_t)state;
    entity.price = price;
    entity.amount = amount;
    entity.total = total;
    entity.raw_id = rawId;
    entity.timeout = timeout;
  }
  */

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
  EBotMarketOrder(EType type, quint64 id) : Entity(type, id) {}

protected:
  Type type;
  QDateTime date;
  double price;
  double amount;
  double fee;
  double total;
  State state;
  //quint64 rawId;
  //quint64 timeout;
};
