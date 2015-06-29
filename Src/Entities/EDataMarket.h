
#pragma once

class EDataMarket : public Entity
{
public:
  static const EType eType = EType::dataMarket;

public:
  EDataMarket(quint64 entityId, const QString& name, const QString& baseCurrency, const QString& commCurrency) : Entity(eType, entityId)
  {
    this->name = name;
    this->baseCurrency = baseCurrency;
    this->commCurrency = commCurrency;
  }

  const QString& getName() const {return name;}
  const QString& getBaseCurrency() const {return baseCurrency;}
  const QString& getCommCurrency() const {return commCurrency;}

private:
  QString name;
  QString baseCurrency;
  QString commCurrency;
};
