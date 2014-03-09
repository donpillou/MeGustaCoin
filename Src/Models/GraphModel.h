
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

  bool synced;
  QList<TradeSample> tradeSamples;
  Bot::Values* values;

  void addTrade(const DataProtocol::Trade& trade);
  
  //double getVwap24() const
  //{
  //  return vwap24;
  //}

signals:
  void dataAdded();

private:
  TradeHandler tradeHander;
  double vwap24;
};
