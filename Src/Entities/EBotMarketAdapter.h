
#pragma once

class EBotMarketAdapter : public Entity
{
public:
  static const EType eType = EType::botMarketAdapter;

public:
  EBotMarketAdapter(meguco_bot_market_entity& data) : Entity(eType, data.entity.id)
  {
    name = DataConnection::getString(data.entity, sizeof(data), data.name_size);
    int x = name.indexOf('/');
    int y = name.indexOf('/', x + 1);
    baseCurrency = name.mid(x + 1, x + 1 - y);
    commCurrency = name.mid(y + 1);
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
