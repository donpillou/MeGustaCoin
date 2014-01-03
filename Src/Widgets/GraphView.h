
#pragma once

class GraphView : public QWidget
{
public:
  GraphView(QWidget* parent, const PublicDataModel* publicDataModel, const QMap<QString, PublicDataModel*>& publicDataModels);

  void setFocusPublicDataModel(const PublicDataModel* publicDataModel);

  void setMaxAge(int maxAge);
  int getMaxAge() const {return maxAge;}

  enum class Data
  {
    trades = 0x01,
    tradeVolume = 0x02,
    orderBook = 0x04,
    regressionLines = 0x08,
    otherMarkets = 0x10,
    all = 0xffff,
  };

  void setEnabledData(unsigned int data);
  unsigned int getEnabledData() const {return enabledData;}

  virtual QSize sizeHint() const;

private:
  const PublicDataModel* publicDataModel;
  const QMap<QString, PublicDataModel*>& publicDataModels;
  const GraphModel* graphModel;

  unsigned int enabledData;

  quint64 time;
  int maxAge;
  double totalMin;
  double totalMax;
  double volumeMax;

  virtual void paintEvent(QPaintEvent* );

  void drawAxesLables(QPainter& painter, const QRect& rect, double hmin, double hmax, const QSize& priceSize);
  void drawTradePolyline(QPainter& painter, const QRect& rect, double hmin, double hmax, double lastVolumeMax, const GraphModel& graphModel, int enabledData, double scale, const QColor& color);
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
