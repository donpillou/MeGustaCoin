
#pragma once

class BitstampMarketStream : public MarketStream
{
public:

  BitstampMarketStream();

  virtual const QString& getMarketCurrency() const {return marketCurrency;}
  virtual const QString& getCoinCurrency() const {return coinCurrency;}
  virtual void loop(Callback& callback);
  virtual void cancel();

private:
  QString marketCurrency;
  QString coinCurrency;
  Websocket websocket;
  bool canceled;
  QMutex canceledConditionMutex;
  QWaitCondition canceledCondition;

  qint64 timeOffset;
  bool timeOffsetSet;

  void sleep(unsigned int ms);
};
