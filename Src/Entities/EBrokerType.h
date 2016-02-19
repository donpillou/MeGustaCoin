
#pragma once

class EBrokerType : public Entity
{
public:
  static const EType eType = EType::brokerType;

public:
  EBrokerType(const meguco_broker_type_entity& data, const QString& name) : Entity(eType, data.entity.id), name(name)
  {
    int x = name.indexOf('/');
    int y = name.indexOf('/', x + 1);
    commCurrency = name.mid(x + 1, y - (x + 1));
    baseCurrency = name.mid(y + 1);
  }

  const QString& getName() const {return name;}
  const QString& getBaseCurrency() const {return baseCurrency;}
  const QString& getCommCurrency() const {return commCurrency;}

  QString formatAmount(double amount) const;
  QString formatPrice(double price) const;

private:
  QString name;
  QString baseCurrency;
  QString commCurrency;
};
