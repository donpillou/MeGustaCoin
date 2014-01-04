
#include "stdafx.h"

MtGoxMarketStream::MtGoxMarketStream() :
  canceled(false), marketCurrency("USD"), coinCurrency("BTC"),
  timeOffsetSet(false), lastTradeId(0) {}

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
  callback.connected();

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
      callback.error(QString("Could not load MtGox/USD server time: %1").arg(httpRequest.getLastError()));
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

    QVariantMap tickerObject = Json::parse(buffer).toMap()["data"].toMap();
    quint64 serverTime = tickerObject["now"].toULongLong() / 1000000ULL;
    quint64 localTime = QDateTime::currentDateTimeUtc().toTime_t();
    timeOffset = (qint64)localTime - (qint64)serverTime;
    timeOffsetSet = true;

    /*
    if(!httpRequest.get("http://data.mtgox.com/api/2/BTCUSD/money/ticker", buffer))
    {
      // todo: warn?
    }
    else
    {
      //{"result":"success","data":{
      //  "high":{"value":"886.31309","value_int":"88631309","display":"$886.31","display_short":"$886.31","currency":"USD"},
      //    "low":{"value":"819.01000","value_int":"81901000","display":"$819.01","display_short":"$819.01","currency":"USD"},
      //    "avg":{"value":"856.52955","value_int":"85652955","display":"$856.53","display_short":"$856.53","currency":"USD"},
      //    "vwap":{"value":"855.22775","value_int":"85522775","display":"$855.23","display_short":"$855.23","currency":"USD"},
      //    "vol":{"value":"15665.20117850","value_int":"1566520117850","display":"15,665.20\u00a0BTC","display_short":"15,665.20\u00a0BTC","currency":"BTC"},
      //    "last_local":{"value":"870.05500","value_int":"87005500","display":"$870.06","display_short":"$870.06","currency":"USD"},
      //    "last_orig":{"value":"870.05500","value_int":"87005500","display":"$870.06","display_short":"$870.06","currency":"USD"},
      //    "last_all":{"value":"870.05500","value_int":"87005500","display":"$870.06","display_short":"$870.06","currency":"USD"},
      //    "last":{"value":"870.05500","value_int":"87005500","display":"$870.06","display_short":"$870.06","currency":"USD"},
      //    "buy":{"value":"876.17567","value_int":"87617567","display":"$876.18","display_short":"$876.18","currency":"USD"},
      //    "sell":{"value":"879.85833","value_int":"87985833","display":"$879.86","display_short":"$879.86","currency":"USD"},
      //    "item":"BTC","now":"1388750714118765"}}
      QVariantMap tickerObject = Json::parse(buffer).toMap()["data"].toMap();
      TickerData tickerData;
      tickerData.date = localTime;
      tickerData.bid = (double)tickerObject["buy"].toMap()["value_int"].toULongLong() / (double)100000ULL;
      tickerData.ask = (double)tickerObject["sell"].toMap()["value_int"].toULongLong() / (double)100000ULL;
      tickerData.high24h = (double)tickerObject["high"].toMap()["value_int"].toULongLong() / (double)100000ULL;
      tickerData.low24h = (double)tickerObject["low"].toMap()["value_int"].toULongLong() / (double)100000ULL;
      tickerData.volume24h = (double)tickerObject["vol"].toMap()["value_int"].toULongLong() / (double)100000000ULL;
      tickerData.vwap24h = (double)tickerObject["vwap"].toMap()["value_int"].toULongLong() / (double)100000ULL;
      callback.receivedTickerData(tickerData);
    }
    */

    if(!httpRequest.get("http://data.mtgox.com/api/2/BTCUSD/money/trades/fetch", buffer))
    {
      callback.error(QString("Could not load MtGox/USD trade history: %1").arg(httpRequest.getLastError()));
      goto cont;
    }

    // {"result":"success","data":[
    // {"date":1388590347,"price":"806.5","amount":"0.2","price_int":"80650000","amount_int":"20000000","tid":"1388590347722119","price_currency":"USD","item":"BTC","trade_type":"ask","primary":"Y","properties":"limit"},
    // {"date":1388590384,"price":"806.5","amount":"0.43920867","price_int":"80650000","amount_int":"43920867","tid":"1388590384943529","price_currency":"USD","item":"BTC","trade_type":"ask","primary":"Y","properties":"market"},
    // ...

    localTime = QDateTime::currentDateTimeUtc().toTime_t();
    QVariantList trades = Json::parse(buffer).toMap()["data"].toList();
    MarketStream::Trade trade;
    for(QVariantList::iterator i = trades.begin(), end = trades.end(); i != end; ++i)
    {
      QVariantMap tradeObject= i->toMap();
      trade.amount =  tradeObject["amount_int"].toDouble() / (double)100000000ULL;;
      trade.price = tradeObject["price_int"].toULongLong() / (double)100000ULL;;
      trade.date = convertTime(localTime, tradeObject["date"].toULongLong());
      quint64 id = tradeObject["tid"].toULongLong();
      QString itemCurrency = tradeObject["item"].toString().toUpper();
      QString priceCurrency = tradeObject["price_currency"].toString().toUpper();

      if(id > lastTradeId && itemCurrency == "BTC" && priceCurrency == "USD")
      {
        callback.receivedTrade(trade);
        lastTradeId = id;
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

    qint64 localTime = QDateTime::currentDateTimeUtc().toTime_t();
    QVariantMap data = Json::parse(buffer).toMap();
    QString channel = data["channel"].toString();

    if(channel == "dbf1dee9-4f2e-4a08-8cb7-748919a71b21") // Trade
    {
      QVariantMap tradeObject = data["trade"].toMap();
      MarketStream::Trade trade;
      trade.amount =  tradeObject["amount_int"].toDouble() / (double)100000000ULL;;
      trade.price = tradeObject["price_int"].toULongLong() / (double)100000ULL;;
      trade.date = convertTime(localTime, tradeObject["date"].toULongLong());
      quint64 id = tradeObject["tid"].toULongLong();
      QString itemCurrency = tradeObject["item"].toString().toUpper();
      QString priceCurrency = tradeObject["price_currency"].toString().toUpper();

      if(id > lastTradeId && itemCurrency == "BTC" && priceCurrency == "USD")
      {
        callback.receivedTrade(trade);
        lastTradeId = id;
      }
    }
    else if(channel == "d5f06780-30a8-4a48-a2f8-7ed181b4a13f") // Ticker
    {
      QVariantMap tickerObject = data["ticker"].toMap();
      TickerData tickerData;
      tickerData.date = convertTime(localTime, tickerObject["now"].toULongLong() / 1000000ULL);
      tickerData.bid = (double)tickerObject["buy"].toMap()["value_int"].toULongLong() / (double)100000ULL;
      tickerData.ask = (double)tickerObject["sell"].toMap()["value_int"].toULongLong() / (double)100000ULL;
      tickerData.high24h = (double)tickerObject["high"].toMap()["value_int"].toULongLong() / (double)100000ULL;
      tickerData.low24h = (double)tickerObject["low"].toMap()["value_int"].toULongLong() / (double)100000ULL;
      tickerData.volume24h = (double)tickerObject["vol"].toMap()["value_int"].toULongLong() / (double)100000000ULL;
      tickerData.vwap24h = (double)tickerObject["vwap"].toMap()["value_int"].toULongLong() / (double)100000ULL;
      callback.receivedTickerData(tickerData);
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

  callback.information("Closed connection to MtGox/USD.");
}

quint64 MtGoxMarketStream::convertTime(quint64 currentLocalTime, quint64 time)
{
  qint64 offset = (qint64)currentLocalTime - (qint64)time;
  if(offset < timeOffset)
    timeOffset = offset;
  return time + timeOffset;
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
