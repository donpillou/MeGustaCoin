
#pragma once

class Bot
{
public:
  enum class Value
  {
    firstRegression, // simple volume weighted linear regression lines
    regression1mA = firstRegression,
    regression1mB,
    regression3mA,
    regression3mB,
    regression5mA,
    regression5mB,
    regression10mA,
    regression10mB,
    regression15mA,
    regression15mB,
    regression20mA,
    regression20mB,
    regression30mA,
    regression30mB,
    regression1hA,
    regression1hB,
    regression2hA,
    regression2hB,
    regression4hA,
    regression4hB,
    regression6hA,
    regression6hB,
    regression12hA,
    regression12hB,
    regression24hA,
    regression24hB,
    numOfRegressions = (regression24hB + 1 - firstRegression) / 2,

    firstBellRegression = firstRegression + numOfRegressions * 2,
    bellRegression1mA = firstBellRegression,
    bellRegression1mB,
    bellRegression3mA,
    bellRegression3mB,
    bellRegression5mA,
    bellRegression5mB,
    bellRegression10mA,
    bellRegression10mB,
    bellRegression15mA,
    bellRegression15mB,
    numOfBellRegressions  = (bellRegression15mB + 1- firstBellRegression) / 2,

    numOfValues = firstBellRegression + numOfBellRegressions * 2,
  };

  class Session
  {
  public:
    virtual ~Session() {};
    virtual void handle(const DataProtocol::Trade& trade, double* values) = 0;
  };

  class Market
  {
  public:
    virtual bool buy(double price, double amount, quint64 timeout) = 0;
    virtual bool sell(double price, double amount, quint64 timeout) = 0;
  };

  virtual Session* createSession(Market& market) = 0;
};

class TradeHandler
{
public:

  double values[(int)Bot::Value::numOfValues];

  void add(const DataProtocol::Trade& trade, bool updateValues)
  {
      quint64 depths[] = {1 * 60, 3 * 60, 5 * 60, 10 * 60, 15 * 60, 20 * 60, 30 * 60, 1 * 60 * 60, 2 * 60 * 60, 4 * 60 * 60, 6 * 60 * 60, 12 * 60 * 60, 24 * 60 * 60};
      for(int i = 0; i < (int)Bot::Value::numOfRegressions; ++i)
      {
        averager[i].add(trade.time, trade.amount, trade.price);
        averager[i].limitToAge(depths[i]);
        if(updateValues)
          averager[i].getLine(values[(int)Bot::Value::firstRegression + i * 2], values[(int)Bot::Value::firstRegression + i * 2 + 1]);
      }

      for(int i = 0; i < (int)Bot::Value::numOfBellRegressions; ++i)
      {
        bellAverager[i].add(trade.time, trade.amount, trade.price, depths[i]);
        if(updateValues)
          bellAverager[i].getLine(depths[i], values[(int)Bot::Value::firstBellRegression + i * 2], values[(int)Bot::Value::firstBellRegression + i * 2 + 1]);
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

    void getLine(double& a, double& b) const
    {
      b = (sumN * sumXY - sumX * sumY) / (sumN * sumXX - sumX * sumX);
      double ar = (sumXX * sumY - sumX * sumXY) / (sumN * sumXX - sumX * sumX);
      a = ar + b * x;
    }

    double getAveragePrice() const
    {
      return sumY / sumN;
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

    void getLine(quint64 ageDeviation, double& a, double& b)
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
      startTime = data.front().time;
    }

    double getAveragePrice() const
    {
      return sumY / sumN;
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

  Averager averager[(int)Bot::Value::numOfRegressions];
  BellAverager bellAverager[(int)Bot::Value::numOfBellRegressions];
};
