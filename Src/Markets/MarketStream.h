
#pragma once

class MarketStream
{
public:
  class Trade;

  class Callback
  {
  public:
    virtual void receivedTrade(const Trade& trade) = 0;
    virtual void error(const QString& message) = 0;
  };

  class Trade
  {
  public:
    double amount;
    quint64 date;
    double price;
  };

  enum class Features
  {
    trades = 0x01,
    orderBook = 0x02,
    //ticker = 0x04,
  };

  virtual ~MarketStream() {};

  virtual const QString& getMarketCurrency() const = 0;
  virtual const QString& getCoinCurrency() const = 0;
  virtual void loop(Callback& callback) = 0;
  virtual void cancel() = 0;
};