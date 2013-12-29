
#include "stdafx.h"
#include <cfloat>

GraphView::GraphView(QWidget* parent, DataModel& dataModel) :
  QWidget(parent),
  dataModel(dataModel), graphModel(dataModel.graphModel),
  enabledData((unsigned int)Data::all), time(0), maxAge(60 * 60),
  totalMin(0.), totalMax(0.), volumeMax(0.)
{
  connect(&graphModel, SIGNAL(dataAdded()), this, SLOT(update()));
}

void GraphView::setMaxAge(int maxAge)
{
  this->maxAge = maxAge;
}

void GraphView::setEnabledData(unsigned int data)
{
  this->enabledData = data;
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

  if(!graphModel.bookSamples.isEmpty())
  {
    GraphModel::BookSample& bookSample = graphModel.bookSamples.back();
    addToMinMax(bookSample.ask);
    addToMinMax(bookSample.bid);
    for(int i = 0; i < (int)GraphModel::BookSample::ComPrice::numOfComPrice; ++i)
      addToMinMax(bookSample.comPrice[i]);
    if(bookSample.time > time)
      time = bookSample.time;
  }

  double lastTotalMin = totalMin;
  double lastTotalMax = totalMax;
  double lastVolumeMax = volumeMax;

  totalMin = DBL_MAX;
  totalMax = 0.;
  volumeMax = 0.;

  double hmin = floor(lastTotalMin);
  double hmax = ceil(qMax(lastTotalMax, hmin + 1.));
  const QSize priceSize = painter.fontMetrics().size(Qt::TextSingleLine, dataModel.formatPrice(hmax == DBL_MAX ? 0. : hmax));

  QRect plotRect(10, 10, rect.width() - (10 + priceSize.width() + 8), rect.height() - 20 - priceSize.height() + 5);
  if(plotRect.width() <= 0 || plotRect.height() <= 0)
    return; // too small

  if(lastTotalMax != 0.)
    drawAxesLables(painter, plotRect, hmin, hmax, priceSize);
  if(enabledData & (int)Data::orderBook && !graphModel.bookSamples.isEmpty())
    drawBookPolyline(painter, plotRect, hmin, hmax);
  if(enabledData & ((int)Data::trades | (int)Data::tradeVolume) && !graphModel.tradeSamples.isEmpty())
    drawTradePolyline(painter, plotRect, hmin, hmax, lastVolumeMax);
  if(enabledData & (int)Data::regressionLines)
    drawRegressionLines(painter, plotRect, hmin, hmax);

  if((totalMin != lastTotalMin || totalMax != lastTotalMax || volumeMax != lastVolumeMax) && totalMax != 0.)
    update();
}

void GraphView::drawAxesLables(QPainter& painter, const QRect& rect, double vmin, double vmax, const QSize& priceSize)
{
  QPen linePen(Qt::gray);
  linePen.setStyle(Qt::DotLine);
  QPen textPen(Qt::darkGray);

  {
    double vrange = vmax - vmin;
    double height = rect.height();
    double vstep = pow(10., ceil(log10(vrange * 20. / height)));
    if(vstep * height / vrange >= 40.f)
      vstep *= 0.5;
    if(vstep * height / vrange >= 40.f)
      vstep *= 0.5;

    for(int i = 0, count = (int)(vrange / vstep); i <= count; ++i)
    {
      QPoint right(rect.right() + 4, rect.bottom() - vstep * i * height / vrange);

      painter.setPen(linePen);
      painter.drawLine(QPoint(0, right.y()), right);
      painter.setPen(textPen);
      painter.drawText(QPoint(right.x() + 2, right.y() + priceSize.height() * 0.5 - 3), dataModel.formatPrice(vmin + i * vstep));
    }
  }

  {
    quint64 hmax = time;
    quint64 hmin = hmax - maxAge;

    double hrange = hmax - hmin;
    double width = rect.width();
    double hstep = pow(10., ceil(log10(hrange * 35. / width)));
    if(hstep * width / hrange >= 70.f)
      hstep *= 0.5;
    if(hstep * width / hrange >= 70.f)
      hstep *= 0.5;
    if(hstep < 60.)
      hstep = 60.;
    else if(hstep > 60. && hstep < (5. * 60.))
      hstep = ceil(hstep / (5. * 60.)) * (5. * 60.);
    else if(hstep > (5. * 60.) && hstep < (15. * 60.))
      hstep = ceil(hstep / (15. * 60.)) * (15. * 60.);
    else if(hstep > (15. * 60.) && hstep < (30. * 60.))
      hstep = ceil(hstep / (30. * 60.)) * (30. * 60.);
    else if(hstep > (30. * 60.) && hstep < (60. * 60.))
      hstep = ceil(hstep / (60. * 60.)) * (60. * 60.);
    else if(hstep > (60. * 60.) && hstep < 2. * 60. * 60.)
      hstep = ceil(hstep / (2. * 60. * 60.)) * (2. * 60. * 60.);
    else if(hstep > (2. * 60. * 60.) && hstep < 3. * 60. * 60.)
      hstep = ceil(hstep / (3. * 60. * 60.)) * (3. * 60. * 60.);
    else if(hstep > (3. * 60. * 60.) && hstep < 6. * 60. * 60.)
      hstep = ceil(hstep / (6. * 60. * 60.)) * (6. * 60. * 60.);
    else if(hstep > (6 * 60. * 60.) && hstep < 12. * 60. * 60.)
      hstep = ceil(hstep / (12. * 60. * 60.)) * (12. * 60. * 60.);
    else if(hstep > (12. * 60. * 60.) && hstep < 24. * 60. * 60.)
      hstep = ceil(hstep / (24. * 60. * 60.)) * (24. * 60. * 60.);

    double hstart = hmax - floor(hmax / hstep) * hstep;

    QString timeFormat = QLocale::system().timeFormat(QLocale::ShortFormat).replace(":ss", "");
    QString formatedTime;
    QDateTime date;
    for(int i = 0; ; ++i)
    {
      QPoint bottom(rect.right() - (hstart + hstep * i) * width / hrange, rect.bottom() + 4);

      if(bottom.x() < rect.left())
        break;

      painter.setPen(linePen);
      painter.drawLine(QPoint(bottom.x(), 0), bottom);
      painter.setPen(textPen);
      date = QDateTime::fromTime_t(hmax - (hstart + hstep * i));
      //date.setTimeSpec(Qt::UTC);
      formatedTime = date.toLocalTime().time().toString(timeFormat);
      const QSize timeSize = painter.fontMetrics().size(Qt::TextSingleLine, formatedTime);
      QPoint textPos(bottom.x() - timeSize.width() * 0.5, rect.bottom() + 2 + timeSize.height());
      if(textPos.x() > 2)
        painter.drawText(textPos, formatedTime);
    }
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

  QPointF* polyData = (QPointF*)alloca(rect.width() * 4 * sizeof(QPointF));
  //QPointF* polyData2 = (QPointF*)alloca(rect.width() * sizeof(QPointF));
  //QPoint* rangeData = (QPoint*)alloca(rect.width() * 2 * sizeof(QPoint));
  QPoint* volumeData = (QPoint*)alloca(rect.width() * 2* sizeof(QPoint));

  {
    QPointF* currentPoint = polyData;
    //QPointF* currentRegPoint = polyData2;
    //QPoint* currentRangePoint = rangeData;
    QPoint* currentVolumePoint = volumeData;

    const QList<GraphModel::TradeSample>& tradeSamples = graphModel.tradeSamples;
    int i = 0, count = tradeSamples.size();
    for(; i < count; ++i) // todo: optimize this
      if(tradeSamples.at(i).time >= vmin) 
        break;
    if(i < count)
    {
      const GraphModel::TradeSample* sample = &tradeSamples.at(i);

      int pixelX = 0;
      quint64 currentTimeMax = vmin + vrange / rect.width();
      double currentMin = sample->min;
      double currentMax = sample->max;
      double currentVolume = 0;
      int currentEntryCount = 0;
      //double currentLastMin = 0.;
      //double currentLastMax = 0.;
      double currentLast = 0.;
      double currentFirst = 0.;
      double lastLast = 0.;
      int lastLeftPixelX = 0;
      //double currentReg = 0.;

      for(;;)
      {
        if(sample->time > currentTimeMax)
          goto addLine;

        //currentLastMin = sample->min;
        //currentLastMax = sample->max;
        if(sample->min < currentMin)
          currentMin = sample->min;
        if(sample->max > currentMax)
          currentMax = sample->max;
        currentVolume += sample->amount;
        currentLast = sample->last;
        //currentReg = sample->weightedLeastSquare;
        ++currentEntryCount;

        if(++i >= count)
          goto addLine;
        sample = &tradeSamples.at(i);
        continue;

      addLine:
        Q_ASSERT(currentPoint - polyData < rect.width() * 4);
        if(currentEntryCount > 0)
        {
          //currentRegPoint->setX(left + pixelX);
          //currentRegPoint->setY(bottom - (currentReg - hmin) * height / hrange);
          //++currentRegPoint;
          /*
          if (currentMax != currentMin)
          {
            currentRangePoint->setX(left + pixelX);
            currentRangePoint->setY(bottom - (currentMax - hmin) * height / hrange);
            ++currentRangePoint;
            currentRangePoint->setX(left + pixelX);
            currentRangePoint->setY(bottom - (currentMin - hmin) * height / hrange);
            ++currentRangePoint;
          }
          */

          currentVolumePoint->setX(left + pixelX);
          currentVolumePoint->setY(bottomInt);
          ++currentVolumePoint;
          currentVolumePoint->setX(left + pixelX);
          currentVolumePoint->setY(bottomInt - currentVolume * (height / 3) / lastVolumeMax);
          ++currentVolumePoint;

          if(currentVolume > volumeMax)
            volumeMax = currentVolume;
          if(currentMin < totalMin)
            totalMin = currentMin;
          if(currentMax > totalMax)
            totalMax = currentMax;

          if(lastLeftPixelX != 0 && lastLeftPixelX != left + pixelX - 1)
          {
            currentPoint->setX(left + pixelX - 1);
            currentPoint->setY(bottom - (lastLast - hmin) * height / hrange);
            ++currentPoint;
          }
          currentPoint->setX(left + pixelX);
          currentPoint->setY(bottom - (currentFirst - hmin) * height / hrange);
          ++currentPoint;
          if(currentFirst - currentMin < currentFirst - currentMax)
          {
            if(currentMin != currentFirst)
            {
              currentPoint->setX(left + pixelX);
              currentPoint->setY(bottom - (currentMin - hmin) * height / hrange);
              ++currentPoint;
            }
            if(currentMax != currentMin)
            {
              currentPoint->setX(left + pixelX);
              currentPoint->setY(bottom - (currentMax - hmin) * height / hrange);
              ++currentPoint;
            }
            if(currentLast != currentMax)
            {
              currentPoint->setX(left + pixelX);
              currentPoint->setY(bottom - (currentLast - hmin) * height / hrange);
              ++currentPoint;
            }
          }
          else
          {
            if(currentMax != currentFirst)
            {
              currentPoint->setX(left + pixelX);
              currentPoint->setY(bottom - (currentMax - hmin) * height / hrange);
              ++currentPoint;
            }
            if(currentMin != currentMax)
            {
              currentPoint->setX(left + pixelX);
              currentPoint->setY(bottom - (currentMin - hmin) * height / hrange);
              ++currentPoint;
            }
            if(currentLast != currentMin)
            {
              currentPoint->setX(left + pixelX);
              currentPoint->setY(bottom - (currentLast - hmin) * height / hrange);
              ++currentPoint;
            }
          }
          lastLast = currentLast;
          lastLeftPixelX = left + pixelX;

          /*
          if(lastLeftPixelX != 0)
          {
            currentPoint->setX(lastLeftPixelX);
            currentPoint->setY(bottom - (lastLast - hmin) * height / hrange);
            ++currentPoint;
            if(left + pixelX - 1 != lastLeftPixelX)
            {
              currentPoint->setX(left + pixelX - 1);
              currentPoint->setY(bottom - (lastLast - hmin) * height / hrange);
              ++currentPoint;
            }

            currentPoint->setX(left + pixelX);
            currentPoint->setY(bottom - (qMax(qMin(lastLast, currentMax), currentMin) - hmin) * height / hrange);
            ++currentPoint;
          }

          lastLast = currentLast;
          lastLeftPixelX = left + pixelX;
          */
        }
        /*else if(currentLastMin != 0.)
        {
          currentPoint->setX(left + pixelX);
          currentPoint->setY(bottom - (currentLastMax - hmin) * height / hrange);
          ++currentPoint;

          currentPoint->setX(left + pixelX);
          currentPoint->setY(bottom - (currentLastMin - hmin) * height / hrange);
          ++currentPoint;

        }
        */

        if(i >= count)
          break;
        ++pixelX;
        currentTimeMax = vmin + vrange * (pixelX + 1) / width;
        currentMin = sample->min;
        currentMax = sample->max;
        currentEntryCount = 0;
        currentVolume = 0;
        currentFirst = sample->first;
      }
    }

    Q_ASSERT(currentPoint - polyData <= rect.width() * 4);
    //Q_ASSERT(currentRangePoint - rangeData <= rect.width() * 2);
    Q_ASSERT(currentVolumePoint - volumeData <= rect.width() * 2);

    if(enabledData & (int)Data::tradeVolume)
    {
      QPen volumePen(Qt::darkGreen);
      painter.setPen(volumePen);
      painter.drawLines(volumeData, (currentVolumePoint - volumeData) / 2);
    }
    
    //painter.setPen(Qt::darkCyan);
    //painter.drawPolyline(polyData2, currentRegPoint - polyData2);

    if(enabledData & (int)Data::trades)
    {
      QPen rangePen(Qt::black);
      rangePen.setWidth(2);
      painter.setPen(rangePen);
      painter.drawPolyline(polyData, currentPoint - polyData);
      //painter.drawLines(rangeData, (currentRangePoint - rangeData) / 2);
    }
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

  for (int type = 0; type < (int)GraphModel::BookSample::ComPrice::numOfComPrice; ++type)
  {
    QPointF* currentPoint = polyData;

    const QList<GraphModel::BookSample>& bookSummaries = graphModel.bookSamples;
    int i = 0, count = bookSummaries.size();
    for(; i < count; ++i) // todo: optimize this
      if(bookSummaries.at(i).time >= vmin) 
        break;
    if(i < count)
    {
      const GraphModel::BookSample* sample = &bookSummaries.at(i);

      int pixelX = 0;
      quint64 currentTimeMax = vmin + vrange / rect.width();
      double currentVal = sample->comPrice[type];
      int currentEntryCount = 0;

      for(;;)
      {
        if(sample->time > currentTimeMax)
          goto addLine;

        currentVal = sample->comPrice[type];
        ++currentEntryCount;
        if(currentVal > totalMax)
          totalMax = currentVal;
        if(currentVal < totalMin)
          totalMin = currentVal;

        if(++i >= count)
          goto addLine;
        sample = &bookSummaries.at(i);
        continue;

      addLine:
        if(currentEntryCount > 0)
        {
          currentPoint->setX(left + pixelX);
          currentPoint->setY(bottom - (currentVal - hmin) * height / hrange);
          ++currentPoint;
        }
        if(i >= count)
          break;
        ++pixelX;
        currentTimeMax = vmin + vrange * (pixelX + 1) / width;
        currentVal = sample->comPrice[type];
        currentEntryCount = 0;
      }
    }

    int color = type * 0xff / (int)GraphModel::BookSample::ComPrice::numOfComPrice;
    painter.setPen(QColor(0xff - color, 0, color));
    painter.drawPolyline(polyData, currentPoint - polyData);
  }
}

void GraphView::drawRegressionLines(QPainter& painter, const QRect& rect, double vmin, double vmax)
{
  double vrange = vmax - vmin;
  quint64 hmax = time;
  quint64 hmin = hmax - maxAge;
  quint64 hrange = hmax - hmin;
  quint64 width = rect.width();
  double height = rect.height();

  for(int i = 0; i < (int)GraphModel::RegressionDepth::numOfRegressionDepths; ++i)
  {
    const GraphModel::RegressionLine& rl = graphModel.regressionLines[i];
    quint64 startTime = qMax(rl.startTime, hmin);
    quint64 endTime = rl.endTime;
    double val = rl.a - rl.b * (endTime - startTime);
    QPointF a(rect.left() + (startTime - hmin) * width / hrange, rect.bottom() - (val -  vmin) * height / vrange);
    QPointF b(rect.right() - (time - endTime) * width / hrange, rect.bottom() - (rl.a -  vmin) * height / vrange);

    int color = qMin((int)((0xdd - 0x44) * qMin(qMax(fabs(rl.b / 0.005), 0.), 1.)), 0xdd - 0x44);
    QPen pen(rl.b >= 0 ? QColor(0, color + 0x44, 0) : QColor(color + 0x44, 0, 0));
    pen.setWidth(2);
    painter.setPen(pen);

    painter.drawLine(a, b);
  }
}
