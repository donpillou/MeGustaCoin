
#pragma once

class BitstampMarketStream : public MarketStream
{
public:

  BitstampMarketStream();

  virtual const QString& getMarketCurrency() const {return marketCurrency;}
  virtual const QString& getCoinCurrency() const {return coinCurrency;}
  virtual void process(Callback& callback);
  virtual void cancel();

private:
  QString marketCurrency;
  QString coinCurrency;
  bool canceled;
  QMutex canceledConditionMutex;
  QWaitCondition canceledCondition;

  quint64 lastTradeId;
  QDateTime lastPingTime;

  void sleep(unsigned int ms);
};
