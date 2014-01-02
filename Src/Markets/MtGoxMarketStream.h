
#pragma once


class MtGoxMarketStream : public MarketStream
{
public:

  MtGoxMarketStream();

  virtual const QString& getMarketCurrency() const {return marketCurrency;}
  virtual const QString& getCoinCurrency() const {return coinCurrency;}
  virtual void process(Callback& callback);
  virtual void cancel();

private:
  QString marketCurrency;
  QString coinCurrency;
  bool canceled;

  qint64 timeOffset;
  bool timeOffsetSet;

  QDateTime lastMessageTime;

  bool sendPing(Websocket& websocket);
};
