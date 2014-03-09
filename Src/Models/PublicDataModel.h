
#pragma once

class PublicDataModel : public QObject
{
  Q_OBJECT

public:
  GraphModel graphModel;
  TradeModel tradeModel;
  //BookModel bookModel;

  PublicDataModel();

  void setMarket(const QString& marketName);
  void setMarket(const QString& coinCurrency, const QString& marketCurrency);

  const QString& getMarketName() const {return marketName;}
  const QString& getCoinCurrency() const {return coinCurrency;}
  const QString& getMarketCurrency() const {return marketCurrency;}

  QString formatAmount(double amount) const;
  QString formatPrice(double price) const;

  void addTrade(const DataProtocol::Trade& trade);
  quint64 getLastReceivedTradeId() const {return lastReceivedTradeId;}
  void addTicker(quint64 time, double bid, double ask);
  //void setBookData(quint64 time, const QList<MarketStream::OrderBookEntry>& askItems, const QList<MarketStream::OrderBookEntry>& bidItems);

  bool getTicker(double& bid, double& ask) const;

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
  void updatedTicker();

private:
  QString marketName;
  QString coinCurrency;
  QString marketCurrency;
  State state;
  quint64 startTime;
  quint64 lastReceivedTradeId;

  double tickerBid;
  double tickerAsk;
};
