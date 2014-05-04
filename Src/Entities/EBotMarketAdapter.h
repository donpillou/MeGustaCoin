
#pragma once

class EBotMarketAdapter : public Entity
{
public:
  static const EType eType = EType::botMarketAdapter;

public:
  EBotMarketAdapter(BotProtocol::MarketAdapter& data) : Entity(eType, data.entityId)
  {
    name = BotProtocol::getString(data.name);
    baseCurrency = BotProtocol::getString(data.currencyBase);
    commCurrency = BotProtocol::getString(data.currencyComm);
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
