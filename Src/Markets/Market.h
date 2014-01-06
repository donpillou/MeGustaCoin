
#pragma once

class Market
{
public:
  struct Balance
  {
    double reservedUsd; // usd in open orders
    double reservedBtc; // btc in open orders
    double availableUsd; // usd available for orders
    double availableBtc; // btc available for orders
    double fee;

    Balance() : reservedUsd(0.), reservedBtc(0.), availableUsd(0.), availableBtc(0.), fee(0.) {}
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

  virtual ~Market() {};

  virtual const QString& getCoinCurrency() const = 0;
  virtual const QString& getMarketCurrency() const = 0;
  virtual const QString& getLastError() const = 0;

  virtual bool loadOrders(QList<Order>& orders) = 0;
  virtual bool loadBalance(Balance& balance) = 0;
  virtual bool loadTransactions(QList<Transaction>& transactions) = 0;
  virtual bool createOrder(double amount, double price, Market::Order& order) = 0;
  virtual bool cancelOrder(const QString& id) = 0;
  virtual bool createOrderDraft(double amount, double price, Market::Order& order) = 0;
};
