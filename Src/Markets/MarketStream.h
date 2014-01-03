
#pragma once

class MarketStream
{
public:
  class Trade;
  class TickerData;

  class Callback
  {
  public:
    virtual void receivedTickerData(const TickerData& tickerData) = 0;
    virtual void receivedTrade(const Trade& trade) = 0;
    virtual void error(const QString& message) = 0;
    virtual void information(const QString& message) = 0;
  };

  class TickerData
  {
  public:
    quint64 date;
    double bid;
    double ask;
    double high24h;
    double low24h;
    double volume24h;
    double vwap24h; //  volume-weighted average price
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
  virtual void process(Callback& callback) = 0;
  virtual void cancel() = 0;
};
