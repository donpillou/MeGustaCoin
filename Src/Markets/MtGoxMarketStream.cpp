
#include "stdafx.h"

MtGoxMarketStream::MtGoxMarketStream() :
  canceled(false), marketCurrency("USD"), coinCurrency("BTC"),
  timeOffsetSet(false) {}

void MtGoxMarketStream::process(Callback& callback) 
{
  Websocket websocket;
  QByteArray buffer;

  callback.information("Connecting to MtGox/USD...");

  if(!websocket.connect("ws://websocket.mtgox.com:80/mtgox?Currency=USD"))
  {
    callback.error("Could not connect to MtGox/USD.");
    return;
  }

  callback.information("Connected to MtGox/USD.");

  // send subscribe command
  // appearantly i don't need this, but lets send it anyway (just to say hello)
  if(!websocket.send("{\"op\": \"mtgox.subscribe\",\"type\": \"ticker\"}") ||
     !websocket.send("{\"op\": \"mtgox.subscribe\",\"type\": \"trades\"}") ||
     !websocket.send("{\"op\": \"mtgox.subscribe\",\"type\": \"depth\"}"))
  {
    callback.error("Lost connection to MtGox/USD.");
    return;
  }
  lastMessageTime = QDateTime::currentDateTime();

  // message loop
  while(!canceled)
  {
    // wait for update
    if(lastMessageTime.secsTo(QDateTime::currentDateTime()) > 3 * 60)
    {
      if(!sendPing(websocket))
      {
        callback.error("Lost connection to MtGox/USD.");
        return;
      }
      lastMessageTime = QDateTime::currentDateTime();
    }
    if(!websocket.read(buffer, 500))
    {
      callback.error("Lost connection to MtGox/USD.");
      return;
    }
    if(canceled)
      break;

    if(buffer.isEmpty())
      continue;
    lastMessageTime = QDateTime::currentDateTime();

    qint64 now = QDateTime::currentDateTimeUtc().toTime_t();
    //QVariantList data = Json::parseList(buffer);
    QVariant var = Json::parse(buffer);

    //foreach(const QVariant& var, data)
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

  callback.information("Closed connection to MtGox/USD.");
}

bool MtGoxMarketStream::sendPing(Websocket& websocket)
{
  // mtgox websocket server does not support websockt pings. hence lets send some valid commands
  if(!websocket.send("{\"op\": \"unsubscribe\",\"channel\": \"d5f06780-30a8-4a48-a2f8-7ed181b4a13f \"}") ||
     !websocket.send("{\"op\": \"mtgox.subscribe\",\"type\": \"ticker\"}"))
    return false;
  return true;
}

void MtGoxMarketStream::cancel()
{
  canceled = true;
}
