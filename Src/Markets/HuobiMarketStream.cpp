
#include "stdafx.h"

HuobiMarketStream::HuobiMarketStream() :
  canceled(false), marketCurrency("CNY"), coinCurrency("BTC"),
  lastTradeDate(0), loadedTradeHistory(false)
{
  localStartTime = QDateTime::currentDateTimeUtc().toTime_t();
  approxServerStartTime = localStartTime + 8 * 60 * 60; // Hong Kong time
}

void HuobiMarketStream::process(Callback& callback) 
{
  HttpRequest httpRequest;
  bool connected = false;

  callback.information("Connecting to Huobi/CNY...");

  for(; !canceled; sleep(2))
  {
    QByteArray buffer;
    bool result = httpRequest.get("https://detail.huobi.com/staticmarket/detail.html", buffer);
    if(!result)
    {
      if(connected)
        callback.error(QString("Lost connection to Huobi/CNY: %1").arg(httpRequest.getLastError()));
      else
        callback.error(QString("Could not connect to Huobi/CNY: %1").arg(httpRequest.getLastError()));
      return;
    }
    if(!connected)
    {
      connected = true;
      callback.information("Connected to Huobi/CNY.");
      callback.connected();

      quint64 localTime = QDateTime::currentDateTimeUtc().toTime_t();
      QDateTime approxServerTime = QDateTime::fromTime_t(approxServerStartTime + (localTime - localStartTime));
      approxServerTime.setTimeSpec(Qt::UTC);
      QDateTime prevTradeTime = approxServerTime;

      QByteArray buffer2;
      if(httpRequest.get("https://market.huobi.com/market/huobi.php?a=td", buffer2))
      {
        QString chartData(buffer2);
        QStringList chart = chartData.split(QChar('\n'), QString::SkipEmptyParts);
        QStringList entryData;
        QList<MarketStream::Trade> tradeSamples;
        tradeSamples.reserve(chart.size());
        double sumPriceVolume = 0.;
        double sumVolume = 0.;
        double min, max;
        for(int i = chart.size() - 1; i >= 3; --i)
        {
          entryData = chart[i].split(QChar(','));
          if(entryData.size() != 4)
            continue;

          QTime time = QTime::fromString(entryData[0], "hhmmss");
          QDateTime tradeDate(approxServerTime);
          tradeDate.setTime(time);

          int timeDiff = prevTradeTime.secsTo(tradeDate);
          if(qAbs(timeDiff) > 12 * 60 * 60)
            tradeDate = tradeDate.addDays(timeDiff > 0 ? -1 : 1);
          prevTradeTime = tradeDate;

          MarketStream::Trade trade;
          trade.date = tradeDate.toTime_t();
          trade.amount = entryData[2].toDouble();
          trade.price = entryData[1].toDouble();
          tradeSamples.append(trade);

          if(sumVolume == 0.)
            min = max = trade.price;
          sumPriceVolume += trade.price * trade.amount;
          sumVolume += trade.amount;
          if(trade.price < min)
            min = trade.price;
          if(trade.price < max)
            max = trade.price;
        }

        MarketStream::TickerData tickerData;
        tickerData.date = QDateTime::currentDateTimeUtc().toTime_t();
        tickerData.bid = 0.; // todo: grab from buffer1
        tickerData.ask = 0.;  // todo: grab from buffer1
        tickerData.high24h = max; // approx
        tickerData.low24h = min; // approx
        tickerData.volume24h = sumVolume; // approx
        tickerData.vwap24h = sumPriceVolume / sumVolume; // approx
        BitcoinCharts::Data bcData;
        QString bcError;
        if(BitcoinCharts::getData("btcnCNY", bcData, bcError)) // hmm, the approx vwap24h suchs (it's basically a vwap4h)
        { // Since Huobi is not on BitcoinCharts, lets just use BtcChina's data...
          tickerData.high24h = bcData.high;
          tickerData.low24h = bcData.low;
          tickerData.volume24h = bcData.volume;
          tickerData.vwap24h = bcData.vwap24;
        }
        else
        {
          // todo: error or warn
        }
        callback.receivedTickerData(tickerData);

        if(!loadedTradeHistory)
          for(int i = tradeSamples.size() - 1; i >= 0; --i)
          {
            MarketStream::Trade& trade = tradeSamples[i];
            trade.date -= 8 * 60 * 60;
            callback.receivedTrade(trade);
            loadedTradeHistory = true;
          }
      }
      else
      {
        // todo: warn or error
      }

    }
    if(canceled)
      break;

    if(buffer.isEmpty())
    {
      // todo: warn or error
      continue;
    }

    // view_detail({"sells":[
    //{"price":"4388.86","level":1,"amount":0.15},{"price":"4389.001","level":1,"amount":1.4871},{"price":"4389.78","level":1,"amount":1.373},{"price":"4389.789","level":1,"amount":0.3349},{"price":"4389.8","level":1,"amount":0.39},{"price":"4389.97","level":1,"amount":4.664},{"price":"4389.98","level":1,"amount":4},{"price":4390,"level":1,"amount":3.353},{"price":"4390.88","level":1,"amount":0.571},{"price":"4390.9","level":1,"amount":0.5}],
    //"buys":[{"price":"4388.001","level":1,"amount":10.488},{"price":4388,"level":1,"amount":3.7176},{"price":"4387.799","level":1,"amount":4.595},{"price":4387,"level":1,"amount":6.1208},{"price":4386,"level":1,"amount":16},{"price":"4385.301","level":1,"amount":0.4803},{"price":"4385.11","level":1,"amount":2},{"price":"4385.03","level":1,"amount":1},{"price":4385,"level":1,"amount":0.02},{"price":4384,"level":1,"amount":0.04}],
    //"trades":[{"time":"02:34:21","price":4388.001,"amount":0.093,"type":"\u5356\u51fa"},{"time":"02:34:14","price":4388.001,"amount":0.0788,"type":"\u5356\u51fa"},{"time":"02:34:14","price":4388.001,"amount":0.0142,"type":"\u5356\u51fa"},{"time":"02:34:11","price":4389,"amount":0.093,"type":"\u5356\u51fa"},{"time":"02:34:09","price":4389,"amount":2,"type":"\u5356\u51fa"},{"time":"02:34:05","price":4389.001,"amount":1.9829,"type":"\u5356\u51fa"},{"time":"02:34:05","price":4389.001,"amount":0.28,"type":"\u5356\u51fa"},{"time":"02:34:01","price":4389.001,"amount":2,"type":"\u5356\u51fa"},{"time":"02:33:40","price":4389,"amount":1.532,"type":"\u4e70\u5165"},{"time":"02:33:40","price":4389,"amount":0.468,"type":"\u4e70\u5165"},{"time":"02:33:40","price":4389,"amount":3.531,"type":"\u4e70\u5165"},{"time":"02:33:04","price":4389,"amount":0.0196,"type":"\u4e70\u5165"},{"time":"02:33:04","price":4389,"amount":0.039,"type":"\u4e70\u5165"},{"time":"02:33:04","price":4389,"amount":0.21,"type":"\u4e70\u5165"},{"time":"02:33:04","price":4389,"amount":0.711,"type":"\u4e70\u5165"}],
    //"p_new":4388,"level":17.701,"amount":20857,"total":91099837.548766,"amp":0,"p_open":4370.3,"p_high":4395,"p_low":4368.9,"p_last":4370.3,
    //"top_sell":{"4":{"price":"4389.8","level":1,"amount":0.39,"accu":3.735},"3":{"price":"4389.789","level":1,"amount":0.3349,"accu":3.345},"2":{"price":"4389.78","level":1,"amount":1.373,"accu":3.0101},"1":{"price":"4389.001","level":1,"amount":1.4871,"accu":1.6371},"0":{"price":"4388.86","level":1,"amount":0.15,"accu":0.15}},
    //"top_buy":[{"price":"4388.001","level":1,"amount":10.488,"accu":10.488},{"price":4388,"level":1,"amount":3.7176,"accu":14.2056},{"price":"4387.799","level":1,"amount":4.595,"accu":18.8006},{"price":4387,"level":1,"amount":6.1208,"accu":24.9214},{"price":4386,"level":1,"amount":16,"accu":40.9214}]
    //})

    QString dataStr(buffer);
    if(!dataStr.startsWith("view_detail(") || dataStr.length() < 13)
    {
      // todo: warn or error
      continue;
    }

    QStringRef jsonData = dataStr.midRef(12, dataStr.length() - 12 - 1);
    QVariantMap data = Json::parse(jsonData.toAscii()).toMap();

    QVariantList tradesList = data["trades"].toList();

    quint64 localTime = QDateTime::currentDateTimeUtc().toTime_t();
    QDateTime approxServerTime = QDateTime::fromTime_t(approxServerStartTime + (localTime - localStartTime));
    approxServerTime.setTimeSpec(Qt::UTC);

    MarketStream::Trade trade;
    for(int i = tradesList.size() - 1; i >= 0; --i)
    {
      QVariantMap tradeData = tradesList[i].toMap();

      QTime time = QTime::fromString(tradeData["time"].toString(), "hh:mm:ss");
      QDateTime tradeDate(approxServerTime);
      tradeDate.setTime(time);


      int timeDiff = approxServerTime.secsTo(tradeDate);
      if(qAbs(timeDiff) > 12 * 60 * 60)
        tradeDate = tradeDate.addDays(timeDiff > 0 ? -1 : 1);

      trade.date = tradeDate.toTime_t();
      trade.amount = tradeData["amount"].toDouble();
      trade.price = tradeData["price"].toDouble();

      if(trade.date < lastTradeDate)
        continue;
      if(trade.date > lastTradeDate)
        lastTradeList.clear();
      lastTradeDate = trade.date;
      QString tradeStr = tradeData["amount"].toString() + " " + tradeData["price"].toString()+ " " + tradeData["type"].toString();
      if(lastTradeList.contains(tradeStr))
        continue;
      lastTradeList.insert(tradeStr);

      trade.date -= 8 * 60 * 60;
      if(trade.date > localTime)
      {
        approxServerStartTime -= trade.date - localTime;
        trade.date = localTime;
      }

      callback.receivedTrade(trade);
    }
  }

  callback.information("Closed connection to Huobi/CNY.");
}

void HuobiMarketStream::cancel()
{
  canceled = true;
  canceledConditionMutex.lock(),
  canceledCondition.wakeAll();
  canceledConditionMutex.unlock();
}

void HuobiMarketStream::sleep(unsigned int secs)
{
  canceledConditionMutex.lock();
  canceledCondition.wait(&canceledConditionMutex, secs * 1000);
  canceledConditionMutex.unlock();
}
