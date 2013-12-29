
#pragma once

class Market
{
public:
  struct Balance
  {
    double reservedUsd;
    double reservedBtc;
    double availableUsd;
    double availableBtc;
    double fee;

    Balance() : reservedUsd(0.), reservedBtc(0.), availableUsd(0.), availableBtc(0.), fee(0.) {}
  };

  struct TickerData
  {
    double lastTradePrice;
    double highestBuyOrder; // bid
    double lowestSellOrder; // ask

    TickerData() : lastTradePrice(0.), highestBuyOrder(0.), lowestSellOrder(0.) {}
  };

  struct Order
  {
    QString id;
    quint64 date;
    double amount; // amount > 0 => buy, amount < 0 => sell
    double price;
    double fee;
    double total;
  };

  struct Transaction
  {
    QString id;
    quint64 date;
    double amount; // amount > 0 => buy, amount < 0 => sell
    double price;
    double fee;
    double total;
  };

  struct OrderBookEntry
  {
    double amount;
    double price;
  };

  struct Trade
  {
    QString id;
    quint64 date;
    double amount;
    double price;
  };

  virtual ~Market() {};

  virtual const QString& getCoinCurrency() const = 0;
  virtual const QString& getMarketCurrency() const = 0;
  virtual const QString& getLastError() const = 0;

  virtual bool loadOrders(QList<Order>& orders) = 0;
  virtual bool loadBalance(Balance& balance) = 0;
  virtual bool loadTransactions(QList<Transaction>& transactions) = 0;
  virtual bool loadTicker(TickerData& tickerData) = 0;
  virtual bool loadTrades(QList<Trade>& trades) = 0;
  virtual bool loadOrderBook(quint64& date, QList<OrderBookEntry>& bids, QList<OrderBookEntry>& asks) = 0;
  virtual bool createOrder(double amount, double price, Market::Order& order) = 0;
  virtual bool cancelOrder(const QString& id) = 0;

  /*
  virtual double getMaxSellAmout() const = 0;
  virtual double getMaxBuyAmout(double price, double canceledAmount = 0., double canceledPrice = 0.) const = 0;
  */
};
