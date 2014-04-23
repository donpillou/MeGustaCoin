
#pragma once

class EBotMarketAdapter : public Entity
{
public:
  static const EType eType = EType::botMarketAdapter;

public:
  EBotMarketAdapter(quint32 id, BotProtocol::MarketAdapter& data) : Entity(eType, id)
  {
    data.name[sizeof(data.name) - 1] = '\0';
    data.currencyBase[sizeof(data.currencyBase) - 1] = '\0';
    data.currencyComm[sizeof(data.currencyComm) - 1] = '\0';
    name = data.name;
    baseCurrency = data.currencyBase;
    commCurrency = data.currencyComm;
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
