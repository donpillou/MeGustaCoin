
#pragma once

class GraphView : public QWidget
{

public:
  GraphView(QWidget* parent, GraphModel& graphModel);

  void setMarket(Market* market);
  void setMaxAge(int maxAge);

private:
  Market* market;
  GraphModel& graphModel;

  int maxAge;

  virtual void paintEvent(QPaintEvent* );

  void drawBox(QPainter& painter, const QRect& rect);
  void drawAxisLables(QPainter& painter, const QRect& rect, double& hmin, double& hmax, const QSize& priceSize);
  void drawTradePolyline(QPainter& painter, const QRect& rect, double hmin, double hmax);
};
