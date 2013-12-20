
#pragma once

class GraphWidget : public QFrame
{
  Q_OBJECT

public:
  GraphWidget(QWidget* parent, QSettings& settings, DataModel& dataModel);

  void saveState(QSettings& settings);

private slots:
  void setMarket(Market* market);

private:
  Market* market;
  DataModel& dataModel;
  GraphModel& graphModel;

  virtual void paintEvent(QPaintEvent* );

  void drawBox(QPainter& painter, const QRect& rect);
  void drawAxisLables(QPainter& painter, const QRect& rect, double& hmin, double& hmax, const QSize& priceSize);
  void drawTradePolyline(QPainter& painter, const QRect& rect, double hmin, double hmax);
};

