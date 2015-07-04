
#pragma once

class EDataMarket : public Entity
{
public:
  static const EType eType = EType::dataMarket;

public:
  EDataMarket(quint64 entityId, const QString& name) : Entity(eType, entityId)
  {
    this->name = name;
    int firstSlash = name.indexOf('/');
    int secondSlash = name.indexOf('/', firstSlash + 1);
    this->baseCurrency = name.mid(secondSlash + 1);
    this->commCurrency = name.mid(firstSlash + 1, secondSlash - (firstSlash + 1));
  }

  const QString& getName() const {return name;}
  const QString& getBaseCurrency() const {return baseCurrency;}
  const QString& getCommCurrency() const {return commCurrency;}

private:
  QString name;
  QString baseCurrency;
  QString commCurrency;
};
