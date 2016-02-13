
#pragma once

class TradeHandler
{
public:

  enum class Regressions
  {
    regression1m,
    regression3m,
    regression5m,
    regression10m,
    regression15m,
    regression20m,
    regression30m,
    regression1h,
    regression2h,
    regression4h,
    regression6h,
    regression12h,
    regression24h,
    numOfRegressions,
  };

  enum class BellRegressions
  {
    bellRegression1m,
    bellRegression3m,
    bellRegression5m,
    bellRegression10m,
    bellRegression15m,
    bellRegression20m,
    bellRegression30m,
    bellRegression1h,
    bellRegression2h,
    numOfBellRegressions,
  };

  struct Values
  {
    struct RegressionLine
    {
      double price; // a
      double incline; // b
      double average;
    };
    RegressionLine regressions[(int)Regressions::numOfRegressions];
    RegressionLine bellRegressions[(int)BellRegressions::numOfBellRegressions];
  };

  Values values;

  void add(const EMarketTradeData::Trade& trade, quint64 tradeAge)
  {
    static const quint64 depths[] = {1 * 60, 3 * 60, 5 * 60, 10 * 60, 15 * 60, 20 * 60, 30 * 60, 1 * 60 * 60, 2 * 60 * 60, 4 * 60 * 60, 6 * 60 * 60, 12 * 60 * 60, 24 * 60 * 60};

    quint64 tradeAgeSecs = tradeAge / 1000ULL;
    if(tradeAgeSecs > depths[sizeof(depths) / sizeof(*depths) - 1] * 3ULL)
      return;

    bool updateValues = tradeAge == 0;
    quint64 time = trade.time / 1000ULL;

    for(int i = 0; i < (int)Regressions::numOfRegressions; ++i)
    {
      if(tradeAgeSecs <= depths[i])
      {
        averager[i].add(time, trade.amount, trade.price);
        averager[i].limitToAge(depths[i]);
        if(updateValues)
        {
          Values::RegressionLine& rl = values.regressions[i];
          averager[i].getLine(rl.price, rl.incline, rl.average);
        }
      }
    }

    for(int i = 0; i < (int)BellRegressions::numOfBellRegressions; ++i)
    {
      if(tradeAgeSecs <= depths[i] * 3ULL)
      {
        bellAverager[i].add(time, trade.amount, trade.price, depths[i]);
        if(updateValues)
        {
          Values::RegressionLine& rl = values.bellRegressions[i];
          bellAverager[i].getLine(depths[i], rl.price, rl.incline, rl.average);
        }
      }
    }
  }

private:
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
      if(++newCount == (unsigned int)data.size())
        useNewSum();

      data.push_back(DataEntry());
      DataEntry& dataEntry = data.back();
      dataEntry.time = time;
      dataEntry.x = x;
      dataEntry.y = y;
      dataEntry.n = n;
    }

    void limitToVolume(double amount)
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
          if(newCount == (unsigned int)data.size())
            useNewSum();
        }
        else
          dataEntry.n -= nToRemove;
       
        totalNToRemove -= nToRemove;
      }
    }

    void limitToAge(quint64 maxAge)
    {
      if(data.isEmpty())
        return;
      quint64 now = data.back().time;

      while(!data.isEmpty())
      {
        DataEntry& dataEntry = data.front();
        if(now - dataEntry.time <= maxAge)
          return;

        const double& x = dataEntry.x;
        const double& y = dataEntry.y;
        const double& n = dataEntry.n;

        const double nx = n * x;
        const double ny = n * y;
        const double nxy = nx * y;
        const double nxx = nx * x;

        sumXY -= nxy;
        sumY -= ny;
        sumX -= nx;
        sumXX -= nxx;
        sumN -= n;

        data.pop_front();
        if(newCount == (unsigned int)data.size())
          useNewSum();
      }
    }

    void getLine(double& a, double& b, double& avg) const
    {
      b = (sumN * sumXY - sumX * sumY) / (sumN * sumXX - sumX * sumX);
      double ar = (sumXX * sumY - sumX * sumXY) / (sumN * sumXX - sumX * sumX);
      a = ar + b * x;
      avg = sumY / sumN;
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

  class BellAverager
  {
  public:
    BellAverager() : startTime(0), x(0), sumXY(0.), sumY(0.), sumX(0.), sumXX(0.), sumN(0.) {}

    void add(quint64 time, double amount, double price, quint64 ageDeviation)
    {
      if(startTime == 0)
        startTime = time;

      x = time - startTime;
      const double& y = price;
      const double& n = amount;

      data.push_back(DataEntry());
      DataEntry& dataEntry = data.back();
      dataEntry.time = time;
      dataEntry.x = x;
      dataEntry.y = y;
      dataEntry.n = n;

      quint64 ageDeviationTimes3 = ageDeviation * 3;
      while(time - data.front().time > ageDeviationTimes3)
        data.removeFirst();
    }

    void getLine(quint64 ageDeviation, double& a, double& b, double& avg)
    {
      // recompute
      sumXY = sumY = sumX = sumXX = sumN = 0.;
      double deviation = ageDeviation;
      quint64 endTime = data.back().time;
      for(QList<DataEntry>::Iterator i = --data.end(), begin = data.begin();; --i)
      {
        DataEntry& dataEntry = *i;
        double dataAge = endTime - dataEntry.time;
        double dataWeight = qExp(-0.5 * (dataAge * dataAge) / (deviation * deviation));

        if(dataWeight < 0.01)
        {
          int entriesToRemove = i - begin + 1;
          for(int i = 0; i < entriesToRemove; ++i)
            data.removeFirst();
          break;
        }

        const double n = dataEntry.n * dataWeight;
        const double nx = n * dataEntry.x;
        sumXY += nx * dataEntry.y;
        sumY += n * dataEntry.y;
        sumX += nx;
        sumXX += nx * dataEntry.x;
        sumN += n;

        if(i == begin)
          break;
      }

      b = (sumN * sumXY - sumX * sumY) / (sumN * sumXX - sumX * sumX);
      double ar = (sumXX * sumY - sumX * sumXY) / (sumN * sumXX - sumX * sumX);
      a = ar + b * x;
      avg = sumY / sumN;
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
  };

  Averager averager[(int)Regressions::numOfRegressions];
  BellAverager bellAverager[(int)BellRegressions::numOfBellRegressions];
};
