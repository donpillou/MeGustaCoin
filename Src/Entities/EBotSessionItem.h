
#pragma once

class EBotSessionItem : public Entity
{
public:
  static const EType eType = EType::botSessionItem;

  enum class Type
  {
    buy = BotProtocol::Transaction::buy,
    sell = BotProtocol::Transaction::sell,
  };

public:
  EBotSessionItem(BotProtocol::SessionItem& data) : Entity(eType, data.entityId)
  {
    initialType = (Type)data.initialType;
    currentType = (Type)data.currentType;
    date = QDateTime::fromMSecsSinceEpoch(data.date);
    price = data.price;
    amount = data.amount;
    profitablePrice = data.profitablePrice;
    flipPrice = data.flipPrice;
  }

  Type getInitialType() const {return initialType;}
  Type getCurrentType() const {return currentType;}
  const QDateTime& getDate() const {return date;}
  double getPrice() const {return price;}
  double getAmount() const {return amount;}
  double getProfitablePrice() const {return profitablePrice;}
  double getFlipPrice() const {return flipPrice;}

private:
  Type initialType;
  Type currentType;
  QDateTime date;
  double price; // >= 0
  double amount; // >= 0
  double profitablePrice;
  double flipPrice;
};
