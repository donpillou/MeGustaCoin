
#include "stdafx.h"

BtcChinaMarketStream::BtcChinaMarketStream() :
  canceled(false), marketCurrency("CNY"), coinCurrency("BTC"),
  timeOffsetSet(false), lastTid(0) {}

void BtcChinaMarketStream::loop(Callback& callback) 
{
  while(!canceled)
  {
    Download dl;
    char url[256];
    if(lastTid == 0)
      strcpy(url, "https://data.btcchina.com/data/historydata");
    else
      _snprintf(url, sizeof(url), "https://data.btcchina.com/data/historydata?since=%llu", (long long unsigned)lastTid);

    char* buffer = dl.load(url);
    if(canceled)
      break;
    if(!buffer)
    {
      callback.error(QString("Could not update BtcChina trades: ") + dl.getErrorString());
      sleep(10 * 1337);
      continue;
    }

    if(*buffer)
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
