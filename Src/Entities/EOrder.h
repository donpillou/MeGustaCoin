
#pragma once

class EOrder : public Entity
{
public:
  static const EType eType = EType::order;

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

  Type type;
  QDateTime date;
  double amount;
  double price;
  double total;
  State state;

  EOrder(quint32 id) : Entity(eType, id), type(Type::unknown), amount(0.), price(0.), total(0.), state(State::open) {}
};
