
#pragma once

class GraphModel : public QObject
{
  Q_OBJECT

public:
  GraphModel();

  class TradeSample
  {
  public:
    quint64 time;
    double min;
    double max;
    double first;
    double last;
    double amount;

    TradeSample() : amount(0) {}
  };

  enum class Marker
  {
    buyMarker,
    sellMarker,
    buyAttemptMarker,
    sellAttemptMarker,
  };

  QList<TradeSample> tradeSamples;
  Bot::Values* values;
  QMap<quint64, Marker> markers;

  void addTrade(const DataProtocol::Trade& trade, quint64 tradeAge);
  void addMarker(quint64 time, Marker marker);
  void clearMarkers();

signals:
  void dataAdded();

private:
  TradeHandler tradeHander;
};
