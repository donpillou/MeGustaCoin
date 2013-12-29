
#pragma once

class PublicDataModel : public QObject
{
  Q_OBJECT

public:
  GraphModel graphModel;
  TradeModel tradeModel;
  BookModel bookModel;

  PublicDataModel(QObject* parent);

  void setMarket(const QString& marketName, int features);
  void setMarket(const QString& coinCurrency, const QString& marketCurrency);

  const QString& getMarketName() const {return marketName;}
  const QString& getCoinCurrency() const {return coinCurrency;}
  const QString& getMarketCurrency() const {return marketCurrency;}
  int getFeatures() const {return features;}

  QString formatAmount(double amount) const;
  QString formatPrice(double price) const;

  void addTrade(const MarketStream::Trade& trade);

signals:
  void changedMarket();

private:
  QString marketName;
  QString coinCurrency;
  QString marketCurrency;
  int features;
};
