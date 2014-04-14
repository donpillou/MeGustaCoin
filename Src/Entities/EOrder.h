
#pragma once

class EOrder : public Entity
{
public:
  static const EType eType = EType::order;

public:
  enum class Type
  {
    unknown,
    buy,
    sell,
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
  EOrder(quint32 id) : Entity(eType, id), type(Type::unknown), amount(0.), price(0.), total(0.), state(State::open) {}

  Type getType() const {return type;}
  const QDateTime& getDate() const {return date;}
  const double& getAmount() const {return amount;}
  const double& getPrice() const {return price;}
  const double& getTotal() const {return total;}
  State getState() const {return state;}

private:
  Type type;
  QDateTime date;
  double amount;
  double price;
  double total;
  State state;
};