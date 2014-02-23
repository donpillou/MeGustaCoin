
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

  //class TickerSample : public MarketStream::TickerData
  //{
  //public:
  //  TickerSample(const MarketStream::TickerData& tickerData) : MarketStream::TickerData(tickerData) {}
  //};

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
    depth1m,
    depth3m,
    depth5m,
    depth10m,
    depth15m,
    numOfExpRegessionDepths,
    depth20m = numOfExpRegessionDepths,
    depth30m,
    depth1h,
    depth2h,
    depth4h,
    depth6h,
    depth12h,
    depth24h,
    
    /*
    depth10,
    depth20,
    depth50,
    depth100,
    depth200,
    depth500,
    depth1000,
    */
    numOfRegressionDepths
  };

  class RegressionLine
  {
  public:
    double a;
    double b;
    quint64 startTime;
    quint64 endTime;
    double averagePrice;

    RegressionLine() : endTime(0) {}
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

    void getLine(double& a, double& b, quint64& startTime, quint64& endTime) const
    {
      b = (sumN * sumXY - sumX * sumY) / (sumN * sumXX - sumX * sumX);
      double ar = (sumXX * sumY - sumX * sumXY) / (sumN * sumXX - sumX * sumX);
      a = ar + b * x;
      startTime = data.isEmpty() ? startTime : data.front().time;
      endTime = data.isEmpty() ? startTime : data.back().time;
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

  class ExpAverager
  {
  public:
    ExpAverager() : startTime(0), x(0), sumXY(0.), sumY(0.), sumX(0.), sumXX(0.), sumN(0.) {}

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

    void getLine(quint64 ageDeviation, double& a, double& b, quint64& startTime, quint64& endTime)
    {
      // recompute
      sumXY = sumY = sumX = sumXX = sumN = 0.;
      double deviation = ageDeviation;
      endTime = data.back().time;
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


//  class ExpAverager
//  {
//  public:
//    ExpAverager() : startTime(0), endTime(0), x(0), sumXY(0.), sumY(0.), sumX(0.), sumXX(0.), sumN(0.) {}
//
//    void add(quint64 time, double amount, double price, quint64 ageDeviation)
//    {
//      if(startTime == 0)
//        endTime = startTime = time;
//
//      this->ageDeviation = ageDeviation;
//      double deviation = ageDeviation;
//      double dataAge = (time - endTime);
//      double dataWeight = qExp(-0.5 * (dataAge * dataAge) / (deviation * deviation));
//
//      sumXY *= dataWeight;
//      sumY *= dataWeight;
//      sumX *= dataWeight;
//      sumXX *= dataWeight;
//      sumN *= dataWeight;
//
//      x = time - startTime;
//      const double& y = price;
//      const double& n = amount;
//
//      const double nx = n * x;
//      const double ny = n * y;
//      const double nxy = nx * y;
//      const double nxx = nx * x;
//
//      sumXY += nxy;
//      sumY += ny;
//      sumX += nx;
//      sumXX += nxx;
//      sumN += n;
//
//      endTime = time;
//    }
//
//    void getLine(double& a, double& b, quint64& startTime, quint64& endTime) const
//    {
//      b = (sumN * sumXY - sumX * sumY) / (sumN * sumXX - sumX * sumX);
//      double ar = (sumXX * sumY - sumX * sumXY) / (sumN * sumXX - sumX * sumX);
//      a = ar + b * x;
//      startTime = this->endTime - ageDeviation;
//      endTime = this->endTime;
//    }
//
//    double getAveragePrice() const
//    {
//      return sumY / sumN;
//    }
//
//  private:
//    quint64 startTime;
//    quint64 endTime;
//    quint64 ageDeviation;
//    double x;
//    double sumXY;
//    double sumY;
//    double sumX;
//    double sumXX;
//    double sumN;
//  };

  /*
  class Estimator
  {
  public:
    Estimator() : lastTime(0)
    {
      particles[0].incline = 0;
      int i = 1, j = 16 + 1;
      for(int k = 0; k < 16; ++k)
      {
        particles[i++].incline = (k + 1) / (60. * 60.);
        particles[j++].incline = (k + 1) / (-60. * 60.);
      }
    }

    double tradeVariance; // assumed variance of a trade
    double estimateVariance; // assumed variance of the estimate
    quint64 lastTime;
    double estimate;
    double incline;

    void add(quint64 time, double amount, double price)
    {
      if(lastTime == 0)
      {
        for(int i = 0; i < sizeof(particles) / sizeof(*particles); ++i)
        {
          Particle& particle = particles[i];
          particle.variance = estimateVariance;
          particle.mean = price;
        }
        lastTime= time;
        estimate = price;
        incline = 0.;
        return;
      }

      quint64 deltaTime = time - lastTime;
      lastTime= time;
      double maxWeight = 0.;
      int bestParticle;
      for(int i = 0; i < sizeof(particles) / sizeof(*particles); ++i)
      {
        Particle& particle = particles[i];
        particle.predict(deltaTime);
        particle.weight *= particle.getProbabilityOfTrade(price, tradeVariance / amount);
        if(particle.weight < 0.001)
          particle.weight = 0.001;
        particle.integrate(price, amount, tradeVariance);
        if(particle.weight > maxWeight)
        {
          bestParticle = i;
          maxWeight = particle.weight;
        }
      }
      estimate = particles[bestParticle].mean;
      incline = particles[bestParticle].incline;
      for(int i = 0; i < sizeof(particles) / sizeof(*particles); ++i)
      {
        Particle& particle = particles[i];
        particle.mean = estimate;
        particle.weight /= maxWeight;
      }
    }

  private:
    class Particle
    {
    public:
      double incline; // usd per second
      double weight;
      double mean;
      double variance;

      Particle() : weight(1.) {}

      void predict(quint64 deltaTime)
      {
        mean += incline * deltaTime;
      }

      double getProbabilityOfTrade(double price, double tradeVariance)
      {
        double diff = price - mean;
        double exponent = diff * diff / tradeVariance;
        double p = qExp(-0.5 * exponent);
        return qMax(p, 0.0000001);
      }

      void integrate(double price, double amount, double tradeVariance)
      {
        double k = variance / (variance + tradeVariance / amount);
        mean += k * (price - mean);
      }
    };
    Particle particles[33];
  };
  */

  QList<TradeSample> tradeSamples;
  QList<BookSample> bookSamples;
  //QList<TickerSample> tickerSamples;
  RegressionLine regressionLines[(int)RegressionDepth::numOfRegressionDepths];
  RegressionLine expRegressionLines[(int)RegressionDepth::numOfRegressionDepths];
  //Estimator estimations[10];

  void addTrade(quint64 id, quint64 time, double price, double amount, bool isSyncOrLive);
  void addBookSample(const BookSample& bookSummary);
  //void addTickerData(const MarketStream::TickerData& tickerData);

  double getVwap24() const
  {
    //if(!tickerSamples.isEmpty())
    //  return tickerSamples.back().vwap24h;
    if(!tradeSamples.isEmpty())
      return regressionLines[(int)RegressionDepth::depth24h].averagePrice;
    return 0;
  }

signals:
  void dataAdded();

private:
  Averager averager[(int)RegressionDepth::numOfRegressionDepths];
  ExpAverager expAverager[(int)RegressionDepth::numOfExpRegessionDepths];
};
