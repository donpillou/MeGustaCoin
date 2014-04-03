
#pragma once

//class EMarket : public Entity
//{
//public:
//  static const EType type = EType::market;
//
//  EMarket() : Entity(EType::market, 0) {}
//
//  const QString& getBaseCurrency() const {return baseCurrency;}
//  const QString& getCommCurrency() const {return commCurrency;}
//
//  QString formatAmount(double amount) const;
//  QString formatPrice(double price) const;
//
//private:
//  QString baseCurrency;
//  QString commCurrency;
//};

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
