
#include "stdafx.h"

BitstampMarketStream::BitstampMarketStream() :
  canceled(false), marketCurrency("USD"), coinCurrency("BTC") {}

void BitstampMarketStream::process(Callback& callback)
{
  Websocket websocket;
  QByteArray buffer;
  
  callback.information("Connecting to Bitstamp/USD...");

  if(!websocket.connect("ws://ws.pusherapp.com/app/de504dc5763aeef9ff52?protocol=6&client=js&version=2.1.2"))
  {
    callback.error("Could not connect to Bitstamp/USD.");
    return;
  }

  callback.information("Connected to Bitstamp/USD.");

  // send subscribe command
  QByteArray message("{\"event\":\"pusher:subscribe\",\"data\":{\"channel\":\"live_trades\"}}");
  if(!websocket.send(message))
  {
    callback.error("Lost connection to Bitstamp/USD.");
    return;
  }
  lastMessageTime = QDateTime::currentDateTime();

  // message loop
  while(!canceled)
  {
    // wait for update
    if(lastMessageTime.secsTo(QDateTime::currentDateTime()) > 3 * 60)
    {
      if(!websocket.sendPing())
      {
        callback.error("Lost connection to Bitstamp/USD.");
        return;
      }
      lastMessageTime = QDateTime::currentDateTime();
    }

    if(!websocket.read(buffer, 500))
    {
      callback.error("Lost connection to Bitstamp/USD.");
      return;
    }
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

  callback.information("Closed connection to Bitstamp/USD.");
}

void BitstampMarketStream::cancel()
{
  canceled = true;
}
