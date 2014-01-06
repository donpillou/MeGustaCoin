

#pragma once

class BitcoinCharts
{
public:
  class Data
  {
  public:
    double high;
    double low;
    double volume;
    double vwap24;
    double bid;
    double ask;
    double last; // close
  };

  static bool getData(const QString& symbol, Data& data, QString& error)
  {
    QMutexLocker sync(&mutex);
    if(lastRequest.isNull() || lastRequest.secsTo(QDateTime::currentDateTimeUtc()) > 15 * 60)
    {
      lastRequest = QDateTime::currentDateTimeUtc();
      HttpRequest httpRequest;
      QByteArray buffer;
      if(httpRequest.get("http://api.bitcoincharts.com/v1/markets.json", buffer))
      {
        // [{"high": 754.000000000000, "latest_trade": 1388731892, "bid": 680.000000000000, "volume": 1.286100000000, "currency": "USD", "currency_volume": 940.317571400000, "ask": 779.000000000000, "close": 754.000000000000, "avg": 731.1387694580514734468548324, "symbol": "bitkonanUSD", "low": 680.000000000000},

        QVariantList dataList = Json::parse(buffer).toList();
        cachedData.clear();
        Data data;
        foreach(QVariant var, dataList)
        {
          QVariantMap varMap = var.toMap();
          data.high = varMap["high"].toDouble();
          data.low = varMap["low"].toDouble();
          data.volume = varMap["volume"].toDouble();
          data.vwap24 = varMap["avg"].toDouble();
          data.bid = varMap["bid"].toDouble();
          data.ask = varMap["ask"].toDouble();
          data.last = varMap["close"].toDouble();
          cachedData[varMap["symbol"].toString()] = data;
        }
      }
      else
      {
        lastError = httpRequest.getLastError();
        if(!cachedData.contains(symbol))
        {
          error = lastError;
          return false;
        }
        // todo: warn or error
      }
    }
    if(!cachedData.contains(symbol))
    {
      if(!cachedData.isEmpty())
        error = QString("Could not find symbol %1.").arg(symbol);
      else
        error = lastError;
      return false;
    }
    data = cachedData[symbol];
    return true;
  }

public:
  static QDateTime lastRequest;
  static QHash<QString, Data> cachedData;
  static QString lastError;
  static QMutex mutex;
};
