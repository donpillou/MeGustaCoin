
#pragma once

class GraphModel : public QObject
{
  Q_OBJECT

public:
  class Entry
  {
  public:
    quint64 time;
    double min;
    double max;

    Entry() : min(0), max(0) {}
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

    double comPrice[ComPrice::numOfComPrice]; // order book center of mass limited to 50, 100, ... btc
  };

  double totalMin; // todo: do i really need this?
  double totalMax;

  QMap<quint64, Entry> trades;
  QList<BookSummary> bookSummaries;

  GraphModel() : totalMin(0), totalMax(0) {}

  void addTrade(quint64 time, double price);
  void addBookSummary(const BookSummary& bookSummary);

signals:
  void dataAdded();
};
