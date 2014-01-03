
#pragma once

class PublicDataModel : public QObject
{
  Q_OBJECT

public:
  GraphModel graphModel;
  TradeModel tradeModel;
  BookModel bookModel;
  QColor color;

  PublicDataModel(QObject* parent, const QColor& color);

  void setMarket(const QString& marketName, int features);
  void setMarket(const QString& coinCurrency, const QString& marketCurrency);

  const QString& getMarketName() const {return marketName;}
  const QString& getCoinCurrency() const {return coinCurrency;}
  const QString& getMarketCurrency() const {return marketCurrency;}
  int getFeatures() const {return features;}

  QString formatAmount(double amount) const;
  QString formatPrice(double price) const;

  void addTrade(const MarketStream::Trade& trade);
  void addTickerData(const MarketStream::TickerData& tickerData);
  void setBookData(quint64 time, const QList<Market::OrderBookEntry>& askItems, const QList<Market::OrderBookEntry>& bidItems);

  enum class State
  {
    connecting,
    connected,
    offline,
  };

  State getState() const {return state;}
  QString getStateName() const;
  void setState(State state);

signals:
  void changedMarket();
  void changedState();

private:
  QString marketName;
  QString coinCurrency;
  QString marketCurrency;
  int features;
  State state;
  quint64 startTime;
};
