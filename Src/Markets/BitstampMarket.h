
#pragma once

class BitstampMarket : public Market
{
public:
  BitstampMarket(const QString& userName, const QString& key, const QString& secret);

  virtual const QString& getCoinCurrency() const {return coinCurrency;}
  virtual const QString& getMarketCurrency() const {return marketCurrency;}

  virtual const QString& getLastError() const {return error;}

  virtual bool loadOrders(QList<Order>& orders);
  virtual bool loadBalance(Balance& balance);
  virtual bool loadTransactions(QList<Transaction>& transactions);
  virtual bool loadTicker(TickerData& tickerData);
  virtual bool loadTrades(QList<Trade>& trades);
  virtual bool loadOrderBook(quint64& date, QList<OrderBookEntry>& bids, QList<OrderBookEntry>& asks);

  virtual bool createOrder(double amount, double price, Market::Order& order);
  virtual bool cancelOrder(const QString& id);

private:
  QString coinCurrency;
  QString marketCurrency;

  QString userName;
  QString key;
  QString secret;

  Market::Balance balance;
  bool balanceLoaded;
  QHash<QString, Market::Order> orders;

  QString error;

  HttpRequest httpRequest;

  QDateTime lastRequestTime;
  quint64 lastNonce;
  QDateTime lastLiveTradeUpdateTime;

  bool request(const char* url, bool isPublic, const QVariantMap& params, QVariant& result);

  void avoidSpamming();

  double getOrderCharge(double amount, double price) const;
  double getMaxSellAmout() const;
  double getMaxBuyAmout(double price) const;

  bool loadBalanceAndFee();

  class VariantBugWorkaround : public QVariant
  {
  public:
    ~VariantBugWorkaround();
  };
};
