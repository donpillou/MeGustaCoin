
#pragma once

class EBotSessionTransaction : public Entity
{
public:
  static const EType eType = EType::botSessionTransaction;

  enum class Type
  {
    buy = BotProtocol::Transaction::buy,
    sell = BotProtocol::Transaction::sell,
  };

public:
  EBotSessionTransaction(BotProtocol::Transaction& data) : Entity(eType, data.entityId)
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
  double price; // >= 0
  double amount; // >= 0
  double fee; // >= 0
  double total;
};
