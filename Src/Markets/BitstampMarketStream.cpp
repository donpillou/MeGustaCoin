
#include "stdafx.h"

BitstampMarketStream::BitstampMarketStream() :
  canceled(false), marketCurrency("USD"), coinCurrency("BTC"),
  timeOffsetSet(false) {}

#include <string>
void BitstampMarketStream::loop(Callback& callback) 
{
  QByteArray buffer;
  while(!canceled)
  {
    if(!websocket.isConnected())
    {
      websocket.close();
      if(!websocket.connect("ws://ws.pusherapp.com/app/de504dc5763aeef9ff52?protocol=6&client=js&version=2.1.2"))
      {
        callback.error("Could not connect to Bitstamp's streaming API.");
        sleep(10 * 1000);
        continue;
      }
      if(canceled)
        break;

      // send subscribe command
      QByteArray message("{\"event\":\"pusher:subscribe\",\"data\":{\"channel\":\"live_trades\"}}");
      if(!websocket.send(message))
        continue;
      if(canceled)
        break;
    }

    // wait for update
    if(!websocket.read(buffer, 500))
      continue;
    if(canceled)
      break;

    if(buffer.isEmpty())
      continue;

    //QFile file("omgblah.txt");
    //file.open(QIODevice::WriteOnly | QIODevice::Append);
    //file.write(buffer);

    QVariantList data = Json::parseList(buffer);
    foreach(const QVariant& var, data)
    {
      QVariantMap data = var.toMap();
      QString event = data["event"].toString();
      QString channel = data["channel"].toString();

      if(channel.isEmpty() && event == "pusher:connection_established")
      {
        // k
      }
      else if(channel == "live_trades")
      {
        if(event == "pusher_internal:subscription_succeeded")
        {
          // k
        }
        else if(event == "trade")
        {
          QVariantMap tradeMap = Json::parse(data["data"].toString().toAscii()).toMap();

          MarketStream::Trade trade;
          trade.amount =  tradeMap["amount"].toDouble();
          trade.price = tradeMap["price"].toDouble();
          trade.date = QDateTime::currentDateTimeUtc().toTime_t();

          callback.receivedTrade(trade);
        }
        else
          callback.error(QString("Received unknown event %1 on channel %1").arg(event, channel));
      }
      else
      {
        callback.error(QString("Received message on unknown channel %1").arg(channel));
      }
    }
  }
}

void BitstampMarketStream::cancel()
{
  canceled = true;
  canceledConditionMutex.lock(),
  canceledCondition.wakeAll();
  canceledConditionMutex.unlock();
}

void BitstampMarketStream::sleep(unsigned int ms)
{
  canceledConditionMutex.lock();
  canceledCondition.wait(&canceledConditionMutex, ms);
  canceledConditionMutex.unlock();
}
