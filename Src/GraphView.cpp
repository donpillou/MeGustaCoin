
#include "stdafx.h"
#include <cfloat>

GraphView::GraphView(QWidget* parent, GraphModel& graphModel) : QWidget(parent), graphModel(graphModel), time(0), maxAge(60 * 60), totalMin(0.), totalMax(0.), volumeMax(0.)
{
  connect(&graphModel, SIGNAL(dataAdded()), this, SLOT(update()));
}

void GraphView::setMarket(Market* market)
{
  this->market = market;
}

void GraphView::setMaxAge(int maxAge)
{
  this->maxAge = maxAge;
}

void GraphView::paintEvent(QPaintEvent* event)
{
  QRect rect = this->rect();
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setPen(Qt::black);

  if(!graphModel.tradeSamples.isEmpty())
  {
    GraphModel::TradeSample& tradeSample = graphModel.tradeSamples.back();
    addToMinMax(tradeSample.max);
    addToMinMax(tradeSample.min);
    if(tradeSample.time > time)
      time = tradeSample.time;
  }

  if(!graphModel.bookSummaries.isEmpty())
  {
    GraphModel::BookSummary& bookSummary = graphModel.bookSummaries.back();
    addToMinMax(bookSummary.ask);
    addToMinMax(bookSummary.bid);
    for(int i = 0; i < (int)GraphModel::BookSummary::ComPrice::numOfComPrice; ++i)
      addToMinMax(bookSummary.comPrice[i]);
    if(bookSummary.time > time)
      time = bookSummary.time;
  }

  double lastTotalMin = totalMin;
  double lastTotalMax = totalMax;
  double lastVolumeMax = volumeMax;

  totalMin = DBL_MAX;
  totalMax = 0.;
  volumeMax = 0.;

  double hmin = floor(lastTotalMin);
  double hmax = ceil(qMax(lastTotalMax, hmin + 1.));
  const QSize priceSize = painter.fontMetrics().size(Qt::TextSingleLine, market->formatPrice(hmax == DBL_MAX ? 0. : hmax));

  QRect plotRect(10, 12, rect.width() - (10 + priceSize.width() + 8), rect.height() - 22);
  if(plotRect.width() <= 0 || plotRect.height() <= 0)
    return; // too small

  if(lastTotalMax != 0.)
    drawAxisLables(painter, plotRect, hmin, hmax, priceSize);
  if(!graphModel.tradeSamples.isEmpty())
    drawTradePolyline(painter, plotRect, hmin, hmax, lastVolumeMax);
  if(!graphModel.bookSummaries.isEmpty())
    drawBookPolyline(painter, plotRect, hmin, hmax);

  if((totalMin != lastTotalMin || totalMax != lastTotalMax || volumeMax != lastVolumeMax) && totalMax != 0.)
    update();
}

void GraphView::drawAxisLables(QPainter& painter, const QRect& rect, double hmin, double hmax, const QSize& priceSize)
{
  double hrange = hmax - hmin;
  double height = rect.height();
  double hstep = pow(10., ceil(log10(hrange * 20. / height)));
  if(hstep * height / hrange >= 40.f)
    hstep *= 0.5;
  if(hstep * height / hrange >= 40.f)
    hstep *= 0.5;

  QPen linePen(Qt::gray);
  linePen.setStyle(Qt::DotLine);
  QPen textPen(Qt::darkGray);

  for(int i = 0, count = (int)(hrange / hstep); i <= count; ++i)
  {
    QPoint right(rect.right(), rect.bottom() - hstep * i * height / hrange);

    painter.setPen(linePen);
    painter.drawLine(QPoint(rect.left(), right.y()), right);
    painter.setPen(textPen);
    painter.drawText(QPoint(rect.right() + 5, right.y() + priceSize.height() * 0.5 - 3), market->formatPrice(hmin + i * hstep));
  }
}

void GraphView::drawTradePolyline(QPainter& painter, const QRect& rect, double hmin, double hmax, double lastVolumeMax)
{
  double hrange = hmax - hmin;
  quint64 vmax = time;
  quint64 vmin = vmax - maxAge;
  quint64 vrange = vmax - vmin;
  int left = rect.left();
  double bottom = rect.bottom();
  int bottomInt = rect.bottom();
  double height = rect.height();
  quint64 width = rect.width();


  QPointF* polyData = (QPointF*)alloca(rect.width() * sizeof(QPointF));
  QPoint* volumeData = (QPoint*)alloca(rect.width() * 2* sizeof(QPoint));
  {
    int pixelX = 0;
    QPointF* currentPoint = polyData;
    QPoint* currentVolumePoint = volumeData;
    quint64 currentTimeMax = vmin + vrange / rect.width();
    double currentMin = graphModel.tradeSamples.begin()->min;
    double currentVolume = 0;
    int currentEntryCount = 0;
    double currentLast = 0.;

    foreach(const GraphModel::TradeSample& sample, graphModel.tradeSamples)
    {
      if(sample.time < vmin) // todo: optimize this
        continue;
      while(sample.time > currentTimeMax)
      {
        Q_ASSERT(currentPoint - polyData < rect.width());
        if(currentEntryCount > 0)
        {
          currentPoint->setX(left + pixelX);
          currentPoint->setY(bottom - (currentMin - hmin) * height / hrange);
          ++currentPoint;
          currentVolumePoint->setX(left + pixelX);
          currentVolumePoint->setY(bottomInt);
          ++currentVolumePoint;
          currentVolumePoint->setX(left + pixelX);
          currentVolumePoint->setY(bottomInt - currentVolume * (height / 3) / lastVolumeMax);
          ++currentVolumePoint;
          if(currentVolume > volumeMax)
            volumeMax = currentVolume;
        }
        else if(currentLast != 0.)
        {
          currentPoint->setX(left + pixelX);
          currentPoint->setY(bottom - (currentLast - hmin) * height / hrange);
          ++currentPoint;
        }
        ++pixelX;
        currentTimeMax = vmin + vrange * (pixelX + 1) / width;
        currentMin = sample.min;
        currentEntryCount = 0;
        currentVolume = 0;
      }
      if(sample.min < currentMin)
        currentMin = sample.min;
      currentLast = sample.min;
      currentVolume += sample.amount;
      ++currentEntryCount;
      if(sample.min < totalMin)
        totalMin = sample.min;
    }
    Q_ASSERT(currentPoint - polyData < rect.width());
    currentPoint->setX(left + pixelX);
    currentPoint->setY(bottom - (currentMin - hmin) * height / hrange);
    if(currentEntryCount > 0)
    {
      currentPoint->setX(left + pixelX);
      currentPoint->setY(bottom - (currentMin - hmin) * height / hrange);
      ++currentPoint;
      currentVolumePoint->setX(left + pixelX);
      currentVolumePoint->setY(bottomInt);
      ++currentVolumePoint;
      currentVolumePoint->setX(left + pixelX);
      currentVolumePoint->setY(bottomInt - currentVolume * (height / 3) / lastVolumeMax);
      ++currentVolumePoint;
      if(currentVolume > volumeMax)
        volumeMax = currentVolume;

    }
    else if(currentLast != 0.)
    {
      currentPoint->setX(left + pixelX);
      currentPoint->setY(bottom - (currentLast - hmin) * height / hrange);
      ++currentPoint;
    }

    painter.setPen(Qt::darkGreen);
    painter.drawLines(volumeData, (currentVolumePoint - volumeData) / 2);
    painter.setPen(Qt::black);
    painter.drawPolyline(polyData, currentPoint - polyData);
  }
  {
    int pixelX = 0;
    QPointF* currentPoint = polyData;
    quint64 currentTimeMax = vmin + vrange / rect.width();
    double currentMax = graphModel.tradeSamples.begin()->max;
    int currentEntryCount = 0;
    double currentLast = 0.;

    foreach(const GraphModel::TradeSample& sample, graphModel.tradeSamples)
    {
      if(sample.time < vmin) // todo: optimize this
        continue;
      while(sample.time > currentTimeMax)
      {
        Q_ASSERT(currentPoint - polyData < rect.width());
        if(currentEntryCount > 0)
        {
          currentPoint->setX(left + pixelX);
          currentPoint->setY(bottom - (currentMax - hmin) * height / hrange);
          ++currentPoint;
        }
        else if(currentLast != 0.)
        {
          currentPoint->setX(left + pixelX);
          currentPoint->setY(bottom - (currentLast - hmin) * height / hrange);
          ++currentPoint;
        }
        ++pixelX;
        currentTimeMax = vmin + vrange * (pixelX + 1) / width;
        currentMax = sample.max;
        currentEntryCount = 0;
      }
      if(sample.max > currentMax)
        currentMax = sample.max;
      currentLast = sample.max;
      ++currentEntryCount;
      if(sample.max > totalMax)
        totalMax = sample.max;
    }
    Q_ASSERT(currentPoint - polyData < rect.width());
    if(currentEntryCount > 0)
    {
      currentPoint->setX(left + pixelX);
      currentPoint->setY(bottom - (currentMax - hmin) * height / hrange);
      ++currentPoint;
    }
    else if(currentLast != 0.)
    {
      currentPoint->setX(left + pixelX);
      currentPoint->setY(bottom - (currentLast - hmin) * height / hrange);
      ++currentPoint;
    }

    painter.drawPolyline(polyData, currentPoint - polyData);
  }
  }

void GraphView::drawBookPolyline(QPainter& painter, const QRect& rect, double hmin, double hmax)
{
  double hrange = hmax - hmin;
  quint64 vmax = time;
  quint64 vmin = vmax - maxAge;
  quint64 vrange = vmax - vmin;
  int left = rect.left();
  double bottom = rect.bottom();
  double height = rect.height();
  quint64 width = rect.width();

  QPointF* polyData = (QPointF*)alloca(rect.width() * sizeof(QPointF));

  for (int i = 0; i < (int)GraphModel::BookSummary::ComPrice::numOfComPrice; ++i)
  {
    int pixelX = 0;
    QPointF* currentPoint = polyData;
    quint64 currentTimeMax = vmin + vrange / rect.width();
    double currentVal = graphModel.bookSummaries.front().comPrice[i];
    int currentEntryCount = 0;

    foreach(const GraphModel::BookSummary& summary, graphModel.bookSummaries)
    {
      if(summary.time < vmin) // todo: optimize this
        continue;
      while(summary.time > currentTimeMax)
      {
        Q_ASSERT(currentPoint - polyData < rect.width());
        
        if(currentEntryCount > 0)
        {
          currentPoint->setX(left + pixelX);
          currentPoint->setY(bottom - (currentVal - hmin) * height / hrange);
          ++currentPoint;
        }
        ++pixelX;
        currentTimeMax = vmin + vrange * (pixelX + 1) / width;
        currentVal = summary.comPrice[i];
        currentEntryCount = 0;
      }
      currentVal = summary.comPrice[i];
      ++currentEntryCount;
      if(currentVal > totalMax)
        totalMax = currentVal;
      if(currentVal < totalMin)
        totalMin = currentVal;
    }
    Q_ASSERT(currentPoint - polyData < rect.width());
    if(currentEntryCount > 0)
    {
      currentPoint->setX(left + pixelX);
      currentPoint->setY(bottom - (currentVal - hmin) * height / hrange);
      ++currentPoint;
    }

    int color = i * 0xff / (int)GraphModel::BookSummary::ComPrice::numOfComPrice;
    painter.setPen(QColor(0xff - color, 0, color));
    painter.drawPolyline(polyData, currentPoint - polyData);
  }
}
