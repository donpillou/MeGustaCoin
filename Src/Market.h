
#pragma once

class Market : public QObject
{
  Q_OBJECT

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

  Market() : orderModel(*this) {};
  virtual ~Market() {};

  virtual void loadOrders() = 0;
  virtual void loadBalance() = 0;
  virtual void loadTicker() = 0;
  virtual void createOrder(const QString& draftId, bool sell, double amount, double price) = 0;
  virtual void cancelOrder(const QString& id) = 0;
  virtual void updateOrder(const QString& id, bool sell, double amount, double price) = 0;
  virtual double getMaxSellAmout() const = 0;
  virtual double getMaxBuyAmout(double price) const = 0;

  OrderModel& getOrderModel() {return orderModel;};

  const Balance* getBalance() const {return balance.fee == 0. ? 0 : &balance;}
  const TickerData* getTickerData() const {return tickerData.lastTradePrice == 0. ? 0 : &tickerData;}

  const char* getMarketCurrency() const {return marketCurrency;}
  const char* getCoinCurrency() const {return coinCurrency;}

signals:
  void balanceUpdated();
  void tickerUpdated();

protected:
  OrderModel orderModel;
  const char* marketCurrency; // USD
  const char* coinCurrency; // BTC
  Balance balance;
  TickerData tickerData;
};
