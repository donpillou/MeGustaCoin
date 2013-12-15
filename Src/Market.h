
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

  Market(DataModel& dataModel) : dataModel(dataModel) {};
  virtual ~Market() {};

  virtual void loadOrders() = 0;
  virtual void loadBalance() = 0;
  virtual void loadTicker() = 0;
  virtual void loadTransactions() = 0;
  virtual void createOrder(const QString& draftId, double amount, double price) = 0;
  virtual void cancelOrder(const QString& id, double oldAmount, double oldPrice) = 0;
  virtual void updateOrder(const QString& id, double amount, double price, double oldAmount, double oldPrice) = 0;
  virtual double getMaxSellAmout() const = 0;
  virtual double getMaxBuyAmout(double price, double canceledAmount = 0., double canceledPrice = 0.) const = 0;
  virtual double getOrderCharge(double amount, double price) const = 0;
  virtual QString formatAmount(double amount) const = 0;
  virtual QString formatPrice(double price) const = 0;

  const Balance* getBalance() const {return balance.fee == 0. ? 0 : &balance;}
  const TickerData* getTickerData() const {return tickerData.lastTradePrice == 0. ? 0 : &tickerData;}

signals:
  void balanceUpdated();
  void tickerUpdated();

protected:
  DataModel& dataModel;
  Balance balance;
  TickerData tickerData;
};
