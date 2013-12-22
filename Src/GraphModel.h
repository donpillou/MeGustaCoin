
#pragma once

class GraphModel : public QObject
{
  Q_OBJECT

public:
  class TradeSample
  {
  public:
    quint64 time;
    double min;
    double max;
    double last;
    double amount;

    TradeSample() : min(0), max(0), last(0), amount(0) {}
  };

  class BookSummary
  {
  public:
    quint64 time;
    double bid;
    double ask;

    enum class ComPrice
    {
      comPrice100,
      comPrice200,
      comPrice500,
      comPrice1000,
      numOfComPrice
    };

    double comPrice[(int)ComPrice::numOfComPrice]; // order book center of mass limited to 50, 100, ... btc
  };

  QList<TradeSample> tradeSamples;
  QList<BookSummary> bookSummaries;

  void addTrade(quint64 time, double price, double amount);
  void addBookSummary(const BookSummary& bookSummary);

signals:
  void dataAdded();
};
