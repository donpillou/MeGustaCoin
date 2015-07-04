
#pragma once

class EBotSessionItemDraft;

class EBotSessionItem : public Entity // todo: rename SessionAsset
{
public:
  static const EType eType = EType::botSessionItem;

  enum class Type
  {
    buy = meguco_user_session_asset_buy,
    sell = meguco_user_session_asset_sell,
  };

  enum class State
  {
    submitting = meguco_user_session_asset_submitting,
    waitBuy = meguco_user_session_asset_wait_buy,
    buying = meguco_user_session_asset_buying,
    waitSell = meguco_user_session_asset_wait_sell,
    selling = meguco_user_session_asset_selling,
    draft,
    updating,
    removing,
  };

public:
  EBotSessionItem(const meguco_user_session_asset_entity& data) : Entity(eType, data.entity.id)
  {
    type = (Type)data.type;
    state = (State)data.state;
    date = QDateTime::fromMSecsSinceEpoch(data.entity.time);
    price = data.price;
    investComm = data.invest_comm;
    investBase = data.invest_base;
    balanceComm = data.balance_comm;
    balanceBase = data.balance_base;
    profitablePrice = data.profitable_price;
    flipPrice = data.flip_price;
    orderId = data.order_id;
  }
  EBotSessionItem(quint64 id, const EBotSessionItemDraft& sessionItem);

  Type getType() const {return type;}
  State getState() const {return state;}
  void setState(State state) {this->state = state;}
  const QDateTime& getDate() const {return date;}
  double getPrice() const {return price;}
  double getInvestComm() const {return investComm;}
  double getInvestBase() const {return investBase;}
  double getBalanceComm() const {return balanceComm;}
  void setBalanceComm(double balanceComm) {this->balanceComm = balanceComm;}
  double getBalanceBase() const {return balanceBase;}
  void setBalanceBase(double balanceBase) {this->balanceBase = balanceBase;}
  double getProfitablePrice() const {return profitablePrice;}
  double getFlipPrice() const {return flipPrice;}
  void setFlipPrice(double flipPrice) {this->flipPrice = flipPrice;}
  quint64 getOrderId() const {return orderId;}

protected:
  EBotSessionItem(EType type, quint64 id) : Entity(type, id) {}

protected:
  Type type;
  State state;
  QDateTime date;
  double price; // >= 0
  double investComm; // >= 0
  double investBase; // >= 0
  double balanceComm; // >= 0
  double balanceBase; // >= 0
  double profitablePrice;
  double flipPrice;
  quint64 orderId;
};
