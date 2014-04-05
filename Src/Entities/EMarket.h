
#pragma once

class EMarket : public Entity
{
public:
  static const EType eType = EType::market;

public:
  EMarket() : Entity(eType, 0) {}

  const QString& getBaseCurrency() const {return baseCurrency;}
  const QString& getCommCurrency() const {return commCurrency;}

  QString formatAmount(double amount) const;
  QString formatPrice(double price) const;

private:
  QString baseCurrency;
  QString commCurrency;
};
