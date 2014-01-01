
#include "stdafx.h"

BtcChinaMarketStream::BtcChinaMarketStream() :
  canceled(false), marketCurrency("CNY"), coinCurrency("BTC"),
  timeOffsetSet(false), lastTid(0) {}

void BtcChinaMarketStream::loop(Callback& callback) 
{
  HttpRequest httpRequest;

  while(!canceled)
  {
    QString url = lastTid == 0 ? QString("https://data.btcchina.com/data/historydata") :
      QString("https://data.btcchina.com/data/historydata?since=%1").arg(lastTid);

    QByteArray buffer;
    bool result = httpRequest.get(url, buffer);
    if(canceled)
      break;
    if(!result)
    {
      callback.error(QString("Could not update BtcChina trades: ") + httpRequest.getLastError());
      sleep(10 * 1337);
      continue;
    }

    if(!buffer.isEmpty())
    {
      // [{"date":"1388417186","price":4406.41,"amount":0.016,"tid":"4144642","type":"sell"},{"date":"1388417186","price":4401,"amount":0.973,"tid":"4144643","type":"sell"}]

      qint64 now = QDateTime::currentDateTimeUtc().toTime_t();
      QVariantList varList = Json::parse(buffer).toList();
      if(!varList.isEmpty())
      {
        QVariantMap tradeData = varList.back().toMap();
        quint64 date = tradeData["date"].toULongLong();
        qint64 offset = now - date;
        if(offset < timeOffset || !timeOffsetSet)
        {
          timeOffset = offset;
          timeOffsetSet = true;
        }
      }
      foreach(const QVariant& var, varList)
      {
        QVariantMap tradeData = var.toMap();
        MarketStream::Trade trade;
        trade.date = tradeData["date"].toULongLong() + timeOffset;
        trade.price = tradeData["price"].toDouble();
        trade.amount = tradeData["amount"].toDouble();
        quint64 tid = tradeData["tid"].toULongLong();
        if(tid > lastTid)
        {
          callback.receivedTrade(trade);
          lastTid = tid;
        }
      }
    }
    sleep(10 * 1337);
  }
}

void BtcChinaMarketStream::cancel()
{
  canceled = true;
  canceledConditionMutex.lock(),
  canceledCondition.wakeAll();
  canceledConditionMutex.unlock();
}

void BtcChinaMarketStream::sleep(unsigned int ms)
{
  canceledConditionMutex.lock();
  canceledCondition.wait(&canceledConditionMutex, ms);
  canceledConditionMutex.unlock();
}
