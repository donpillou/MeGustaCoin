
#include "stdafx.h"

MtGoxMarketStream::MtGoxMarketStream() :
  canceled(false), marketCurrency("USD"), coinCurrency("BTC"),
  timeOffsetSet(false), lastTradeId(false) {}

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
  lastPingTime = QDateTime::currentDateTime();

  // load trade history
  sleep(3); // ensure requested trades are covered by the stream
  {
    HttpRequest httpRequest;
    QByteArray buffer;

    if(!httpRequest.get("http://data.mtgox.com/api/2/BTCUSD/money/ticker_fast", buffer))
    {
      // todo: warn?
      goto cont;
    }

    // {"result":"success","data":{
    // "last_local":{"value":"824.00000","value_int":"82400000","display":"$824.00","display_short":"$824.00","currency":"USD"},
    // "last":{"value":"824.00000","value_int":"82400000","display":"$824.00","display_short":"$824.00","currency":"USD"},
    // "last_orig":{"value":"855.19924","value_int":"85519924","display":"CA$855.20","display_short":"CA$855.20","currency":"CAD"},
    // "last_all":{"value":"803.90244","value_int":"80390244","display":"$803.90","display_short":"$803.90","currency":"USD"},
    // "buy":{"value":"824.00000","value_int":"82400000","display":"$824.00","display_short":"$824.00","currency":"USD"},
    // "sell":{"value":"824.99999","value_int":"82499999","display":"$825.00","display_short":"$825.00","currency":"USD"},
    // "now":"1388675661812776"}}

    QVariantMap tickerData = Json::parse(buffer).toMap()["data"].toMap();
    quint64 serverTime = tickerData["now"].toULongLong() / 1000000ULL;
    quint64 localTime = QDateTime::currentDateTimeUtc().toTime_t();
    qint64 timeOffset = (qint64)localTime - (qint64)serverTime;

    //if(!httpRequest.get(QString("http://data.mtgox.com/api/2/BTCUSD/money/trades/fetch?since=%1").arg((serverTime - 60 * 60) * 1000000ULL), buffer))
    if(!httpRequest.get("http://data.mtgox.com/api/2/BTCUSD/money/trades/fetch", buffer))
    {
      // todo: warn?
      goto cont;
    }

    // {"result":"success","data":[
    // {"date":1388590347,"price":"806.5","amount":"0.2","price_int":"80650000","amount_int":"20000000","tid":"1388590347722119","price_currency":"USD","item":"BTC","trade_type":"ask","primary":"Y","properties":"limit"},
    // {"date":1388590384,"price":"806.5","amount":"0.43920867","price_int":"80650000","amount_int":"43920867","tid":"1388590384943529","price_currency":"USD","item":"BTC","trade_type":"ask","primary":"Y","properties":"market"},
    // ...

    QVariantList trades = Json::parse(buffer).toMap()["data"].toList();
    MarketStream::Trade trade;
    for(QVariantList::iterator i = trades.begin(), end = trades.end(); i != end; ++i)
    {
      QVariantMap tradeData = i->toMap();
      trade.amount =  tradeData["amount_int"].toDouble() / (double)100000000ULL;;
      trade.price = tradeData["price_int"].toULongLong() / (double)100000ULL;;
      trade.date = (qint64)tradeData["date"].toULongLong() + timeOffset;
      //if(localTime - trade.date > 60 * 60)
      //  continue;

      quint64 id = tradeData["tid"].toULongLong();
      if(id > lastTradeId)
        if(tradeData["item"].toString().toUpper() == "BTC" && tradeData["price_currency"].toString().toUpper() == "USD")
        {
          lastTradeId = id;
          callback.receivedTrade(trade);
        }
    }
  }
cont:

  // message loop
  while(!canceled)
  {
    // wait for update
    if(lastPingTime.secsTo(QDateTime::currentDateTime()) > 3 * 60)
    {
      if(!sendPing(websocket))
      {
        callback.error("Lost connection to MtGox/USD.");
        return;
      }
      lastPingTime = QDateTime::currentDateTime();
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
    lastPingTime = QDateTime::currentDateTime();

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

        quint64 id = tradeMap["tid"].toULongLong();
        if(id > lastTradeId)
          if(tradeMap["item"].toString().toUpper() == "BTC" && tradeMap["price_currency"].toString().toUpper() == "USD")
            callback.receivedTrade(trade);
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
  canceledConditionMutex.lock(),
  canceledCondition.wakeAll();
  canceledConditionMutex.unlock();
}

void MtGoxMarketStream::sleep(unsigned int secs)
{
  canceledConditionMutex.lock();
  canceledCondition.wait(&canceledConditionMutex, secs * 1000);
  canceledConditionMutex.unlock();
}
