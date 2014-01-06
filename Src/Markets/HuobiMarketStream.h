
#pragma once

class HuobiMarketStream : public MarketStream
{
public:

  HuobiMarketStream();

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

  quint64 approxServerStartTime;
  quint64 localStartTime;

  QList<QString> lastTradeList;
  bool loadedTradeHistory;
  QDateTime lastTickerUpdate;

  void sleep(unsigned int secs);
};
