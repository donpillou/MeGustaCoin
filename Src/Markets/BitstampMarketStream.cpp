
#include "stdafx.h"

BitstampMarketStream::BitstampMarketStream() :
  canceled(false), marketCurrency("USD"), coinCurrency("BTC"),
  lastTradeId(0) {}

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
  lastPingTime = QDateTime::currentDateTime();

  // load trade history
  sleep(3); // ensure requested trades are covered by the stream
  {
    HttpRequest httpRequest;
    QByteArray buffer;

    if(!httpRequest.get("https://www.bitstamp.net/api/ticker/", buffer))
    {
      // todo: warn?
      goto cont;
    }

    // {"high": "759.90", "last": "753.40", "timestamp": "1388668694", "bid": "753.31", "volume": "5972.35893309", "low": "741.00", "ask": "753.40"}

    QVariantMap tickerData = Json::parse(buffer).toMap();
    quint64 serverTime = tickerData["timestamp"].toULongLong(); // + up to 8 seconds
    quint64 localTime = QDateTime::currentDateTimeUtc().toTime_t();
    qint64 timeOffset = (qint64)localTime - (qint64)serverTime;

    if(!httpRequest.get("https://www.bitstamp.net/api/transactions/", buffer))
    {
      // todo: warn?
      goto cont;
    }

    // [{"date": "1388668244", "tid": 2831838, "price": "754.00", "amount": "0.01326260"}, 
    // {"date": "1388668244", "tid": 2831837, "price": "754.00", "amount": "0.49916678"},
    // ....

    QVariantList trades = Json::parse(buffer).toList();
    MarketStream::Trade trade;
    if(!trades.empty())
      for(QVariantList::iterator i = trades.end() - 1;; --i)
      {
        QVariantMap tradeData = i->toMap();
        trade.amount =  tradeData["amount"].toDouble();
        trade.price = tradeData["price"].toDouble();
        trade.date = (qint64)tradeData["date"].toULongLong() + timeOffset;
        quint64 id = tradeData["tid"].toULongLong();
        if(id > lastTradeId)
        {
          lastTradeId = id;
          callback.receivedTrade(trade);
        }
        if(i == trades.begin())
          break;
      }
  }
cont:

  // message loop
  while(!canceled)
  {
    // wait for update
    if(lastPingTime.secsTo(QDateTime::currentDateTime()) > 120)
    {
      if(!websocket.sendPing())
      {
        callback.error("Lost connection to Bitstamp/USD.");
        return;
      }
      lastPingTime = QDateTime::currentDateTime();
    }

    if(!websocket.read(buffer, 500))
    {
      callback.error("Lost connection to Bitstamp/USD.");
      return;
    }
    if(canceled)
      break;

    if(buffer.isEmpty())
      continue;
    lastPingTime = QDateTime::currentDateTime();

    // {"event":"pusher:connection_established","data":"{\"socket_id\":\"32831.40191965\"}"}
    // {"event":"pusher_internal:subscription_succeeded","data":"{}","channel":"live_trades"}
    // {"event":"trade","data":"{\"price\": 719.98000000000002, \"amount\": 5.3522414999999999, \"id\": 2799842}","channel":"live_trades"}
    // {"event":"trade","data":"{\"price\": 719.99000000000001, \"amount\": 39.6419985, \"id\": 2799843}","channel":"live_trades"}

    quint64 now = QDateTime::currentDateTimeUtc().toTime_t();
    //QVariantList data = Json::parseList(buffer);
    QVariant var = Json::parse(buffer);
    //foreach(const QVariant& var, data)
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
          quint64 id = tradeMap["id"].toULongLong();
          if(id >= lastTradeId)
          {
            lastTradeId = id;
            callback.receivedTrade(trade);
          }
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
  canceledConditionMutex.lock(),
  canceledCondition.wakeAll();
  canceledConditionMutex.unlock();
}

void BitstampMarketStream::sleep(unsigned int secs)
{
  canceledConditionMutex.lock();
  canceledCondition.wait(&canceledConditionMutex, secs * 1000);
  canceledConditionMutex.unlock();
}
