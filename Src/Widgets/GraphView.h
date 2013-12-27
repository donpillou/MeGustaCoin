
#pragma once

class GraphView : public QWidget
{

public:
  GraphView(QWidget* parent, GraphModel& graphModel);

  void setMarket(Market* market);
  void setMaxAge(int maxAge);

  enum class Data
  {
    trades = 0x01,
    tradeVolume = 0x02,
    orderBook = 0x04,
    regressionLines = 0x08,
    all = 0xffff,
  };

  void setEnabledData(unsigned int data);
  unsigned int getEnabledData() const {return enabledData;}

private:
  Market* market;
  GraphModel& graphModel;

  unsigned int enabledData;

  quint64 time;
  int maxAge;
  double totalMin;
  double totalMax;
  double volumeMax;

  virtual void paintEvent(QPaintEvent* );

  void drawAxesLables(QPainter& painter, const QRect& rect, double hmin, double hmax, const QSize& priceSize);
  void drawTradePolyline(QPainter& painter, const QRect& rect, double hmin, double hmax, double lastVolumeMax);
  void drawBookPolyline(QPainter& painter, const QRect& rect, double hmin, double hmax);
  void drawRegressionLines(QPainter& painter, const QRect& rect, double hmin, double hmax);

  inline void addToMinMax(double price)
  {
    if(price > totalMax)
      totalMax = price;
    if(price < totalMin)
      totalMin = price;
  }
};
