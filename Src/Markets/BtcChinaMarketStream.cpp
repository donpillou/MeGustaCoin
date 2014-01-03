
#include "stdafx.h"

BtcChinaMarketStream::BtcChinaMarketStream() :
  canceled(false), marketCurrency("CNY"), coinCurrency("BTC"),
  timeOffsetSet(false), lastTid(0) {}

void BtcChinaMarketStream::process(Callback& callback) 
{
  HttpRequest httpRequest;
  bool connected = false;

  callback.information("Connecting to BtcChina/CNY...");

  for(; !canceled; sleep(14))
  {
    QString url = lastTid == 0 ? QString("https://data.btcchina.com/data/historydata") :
      QString("https://data.btcchina.com/data/historydata?since=%1").arg(lastTid);

    QByteArray buffer;
    bool result = httpRequest.get(url, buffer);
    if(!result)
    {
      if(connected)
        callback.error(QString("Lost connection to BtcChina/CNY: %1").arg(httpRequest.getLastError()));
      else
        callback.error(QString("Could not connect to BtcChina/CNY: %1").arg(httpRequest.getLastError()));
      return;
    }
    if(!connected)
    {
      connected = true;
      callback.information("Connected to BtcChina/CNY.");
      callback.connected();

      BitcoinCharts::Data bcData;
      QString bcError;
      if(BitcoinCharts::getData("btcnCNY", bcData, bcError))
      {
        TickerData tickerData;
        tickerData.date = QDateTime::currentDateTimeUtc().toTime_t();
        tickerData.vwap24h = bcData.vwap24;
        tickerData.bid = bcData.bid;
        tickerData.ask = bcData.ask;
        tickerData.high24h = bcData.high;
        tickerData.low24h = bcData.low;
        tickerData.volume24h = bcData.volume;
        tickerData.vwap24h = bcData.vwap24;

        callback.receivedTickerData(tickerData);
      }
      else
      {
        // todo: warn or error
      }
    }
    if(canceled)
      break;

    if(buffer.isEmpty())
      continue;

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

  callback.information("Closed connection to BtcChina/CNY.");
}

void BtcChinaMarketStream::cancel()
{
  canceled = true;
  canceledConditionMutex.lock(),
  canceledCondition.wakeAll();
  canceledConditionMutex.unlock();
}

void BtcChinaMarketStream::sleep(unsigned int secs)
{
  canceledConditionMutex.lock();
  canceledCondition.wait(&canceledConditionMutex, secs * 1000);
  canceledConditionMutex.unlock();
}
