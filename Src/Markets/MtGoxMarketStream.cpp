
#include "stdafx.h"

MtGoxMarketStream::MtGoxMarketStream() :
  canceled(false), marketCurrency("USD"), coinCurrency("BTC"),
  timeOffsetSet(false) {}

void MtGoxMarketStream::loop(Callback& callback) 
{
  QByteArray buffer;
  while(!canceled)
  {
    if(!websocket.isConnected())
    {
      websocket.close();
      if(!websocket.connect("ws://websocket.mtgox.com:80/mtgox?Currency=USD"))
      {
        callback.error("Could not connect to MtGox' streaming API.");
        sleep(10 * 1000);
        continue;
      }
      if(canceled)
        break;
      // send subscribe command
      // appearantly i don't need this
      //if(!websocket.send())
      //  continue;
      //if(canceled)
      //  break;
    }

    // wait for update
    if(!websocket.read(buffer, 500))
      continue;
    if(canceled)
      break;

    if(buffer.isEmpty())
      continue;

    QVariantList data = Json::parseList(buffer);

    foreach(const QVariant& var, data)
    {
      QVariantMap data = var.toMap();
      QString channel = data["channel"].toString();

      if(channel == "dbf1dee9-4f2e-4a08-8cb7-748919a71b21") // Trade
      {
        QVariantMap tradeMap = data["trade"].toMap();

        if(tradeMap["item"].toString().toUpper() == "BTC" && tradeMap["price_currency"].toString().toUpper() == "USD")
        {
          MarketStream::Trade trade;
          trade.amount =  (double)tradeMap["amount_int"].toULongLong() / (double)100000000ULL;
          trade.price = (double)tradeMap["price_int"].toULongLong() / (double)100000ULL;
          trade.date = tradeMap["date"].toULongLong();

          qint64 now = QDateTime::currentDateTimeUtc().toTime_t();
          qint64 offset = now - trade.date;
          if(offset < timeOffset || !timeOffsetSet)
          {
            timeOffset = offset;
            timeOffsetSet = true;
          }
          trade.date += timeOffset;

          callback.receivedTrade(trade);
        }
      }
      else if(channel == "d5f06780-30a8-4a48-a2f8-7ed181b4a13f") // Ticker
      {
        QVariantMap tickerMap = data["ticker"].toMap();
        // todo
      }
      else if(channel == "24e67e0d-1cad-4cc0-9e7a-f8523ef460fe") // Depth 
      {
        QVariantMap depthMap = data["depth"].toMap();
        // todo
      }
      else
      {
        callback.error(QString("Received message on unknown channel %1").arg(channel));
      }
    }
  }
}

void MtGoxMarketStream::cancel()
{
  canceled = true;
  canceledConditionMutex.lock(),
  canceledCondition.wakeAll();
  canceledConditionMutex.unlock();
}

void MtGoxMarketStream::sleep(unsigned int ms)
{
  canceledConditionMutex.lock();
  canceledCondition.wait(&canceledConditionMutex, ms);
  canceledConditionMutex.unlock();
}
