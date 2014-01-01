
#include "stdafx.h"

BitstampMarketStream::BitstampMarketStream() :
  canceled(false), marketCurrency("USD"), coinCurrency("BTC") {}

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
      lastMessageTime = QDateTime::currentDateTime();
    }

    // wait for update
    if(lastMessageTime.secsTo(QDateTime::currentDateTime()) > 3 * 60)
    {
      if(!websocket.sendPing())
        continue;
      lastMessageTime = QDateTime::currentDateTime();
    }
    if(!websocket.read(buffer, 500))
      continue;
    if(canceled)
      break;

    lastMessageTime = QDateTime::currentDateTime();
    if(buffer.isEmpty())
      continue;

    // {"event":"pusher:connection_established","data":"{\"socket_id\":\"32831.40191965\"}"}
    // {"event":"pusher_internal:subscription_succeeded","data":"{}","channel":"live_trades"}
    // {"event":"trade","data":"{\"price\": 719.98000000000002, \"amount\": 5.3522414999999999, \"id\": 2799842}","channel":"live_trades"}
    // {"event":"trade","data":"{\"price\": 719.99000000000001, \"amount\": 39.6419985, \"id\": 2799843}","channel":"live_trades"}

    quint64 now = QDateTime::currentDateTimeUtc().toTime_t();
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
          trade.date = now;

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
