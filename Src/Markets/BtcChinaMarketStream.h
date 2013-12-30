
#pragma once

class BtcChinaMarketStream : public MarketStream
{
public:

  BtcChinaMarketStream();

  virtual const QString& getMarketCurrency() const {return marketCurrency;}
  virtual const QString& getCoinCurrency() const {return coinCurrency;}
  virtual void loop(Callback& callback);
  virtual void cancel();

private:
  QString marketCurrency;
  QString coinCurrency;
  bool canceled;
  QMutex canceledConditionMutex;
  QWaitCondition canceledCondition;

  qint64 timeOffset;
  bool timeOffsetSet;

  quint64 lastTid;

  void sleep(unsigned int ms);
};
