
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
    double first;
    double last;
    double amount;

    TradeSample() : amount(0) {}
  };

  class BookSample
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

  enum class RegressionDepth
  {
    depth10,
    depth20,
    depth50,
    depth100,
    depth200,
    depth500,
    depth1000,
    numOfRegressionDepths
  };

  class RegressionLine
  {
  public:
    double a;
    double b;
    quint64 startTime;
    quint64 endTime;
  };

  class Averager
  {
  public:
    Averager() : startTime(0), x(0), sumXY(0.), sumY(0.), sumX(0.), sumXX(0.), sumN(0.), newSumXY(0.), newSumY(0.), newSumX(0.), newSumXX(0.), newSumN(0.) {}

    void add(quint64 time, double amount, double price)
    {
      if(startTime == 0)
        startTime = time;

      x = time - startTime;
      const double& y = price;
      const double& n = amount;

      const double nx = n * x;
      const double ny = n * y;
      const double nxy = nx * y;
      const double nxx = nx * x;

      sumXY += nxy;
      sumY += ny;
      sumX += nx;
      sumXX += nxx;
      sumN += n;

      newSumXY += nxy;
      newSumY += ny;
      newSumX += nx;
      newSumXX += nxx;
      newSumN += n;
      if(++newCount == data.size())
        useNewSum();

      data.push_back(DataEntry());
      DataEntry& dataEntry = data.back();
      dataEntry.time = time;
      dataEntry.x = x;
      dataEntry.y = y;
      dataEntry.n = n;
    }

    void limitTo(double amount)
    {
      double totalNToRemove = sumN - amount;
      while(totalNToRemove > 0. && !data.isEmpty())
      {
        DataEntry& dataEntry = data.front();
        double nToRemove= qMin(dataEntry.n, totalNToRemove);

        const double& x = dataEntry.x;
        const double& y = dataEntry.y;
        const double& n = nToRemove;

        const double nx = n * x;
        const double ny = n * y;
        const double nxy = nx * y;
        const double nxx = nx * x;

        sumXY -= nxy;
        sumY -= ny;
        sumX -= nx;
        sumXX -= nxx;
        sumN -= n;

        if(nToRemove >= dataEntry.n)
        {
          data.pop_front();
          if(newCount == data.size())
            useNewSum();
        }
        else
          dataEntry.n -= nToRemove;
       
        totalNToRemove -= nToRemove;
      }
    }

    void getLine(double& a, double& b, quint64& startTime, quint64& endTime)
    {
      b = (sumN * sumXY - sumX * sumY) / (sumN * sumXX - sumX * sumX);
      double ar = (sumXX * sumY - sumX * sumXY) / (sumN * sumXX - sumX * sumX);
      a = ar + b * x;
      startTime = data.isEmpty() ? startTime : data.front().time;
      endTime = data.isEmpty() ? startTime : data.back().time;
    }

  private:
    struct DataEntry
    {
      quint64 time;
      double x;
      double n;
      double y;
    };
    QList<DataEntry> data;
    quint64 startTime;
    double x;
    double sumXY;
    double sumY;
    double sumX;
    double sumXX;
    double sumN;
    double newSumXY;
    double newSumY;
    double newSumX;
    double newSumXX;
    double newSumN;
    unsigned newCount;

    void useNewSum()
    {
      sumXY = newSumXY;
      sumY = newSumY;
      sumX = newSumX;
      sumXX = newSumXX;
      sumN = newSumN;

      sumXY = 0;
      sumY = 0;
      sumX = 0;
      sumXX = 0;
      sumN = 0;
      newCount = 0;
    }
  };

  QList<TradeSample> tradeSamples;
  QList<BookSample> bookSamples;
  RegressionLine regressionLines[(int)RegressionDepth::numOfRegressionDepths];

  void addTrade(quint64 time, double price, double amount);
  void addBookSample(const BookSample& bookSummary);

signals:
  void dataAdded();

private:
  Averager averager[(int)RegressionDepth::numOfRegressionDepths];
};
