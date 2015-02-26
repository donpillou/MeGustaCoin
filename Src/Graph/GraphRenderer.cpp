
#include "stdafx.h"
#include <cfloat>

GraphRenderer::GraphRenderer(const QString& channelName) : channelName(channelName), values(0), enabled(false), upToDate(false), enabledData(trades | expRegressionLines), height(0), width(0), image(0), time(0), ownTime(0),
  maxAge(60 * 60), totalMin(DBL_MAX), totalMax(0.), volumeMax(0.)
{
  tradeSamples.reserve(7 * 24 * 60 * 60 + 1000);
  lowResTradeSamples.reserve(6 * 31 * 24 * 60 * 60 / 300 + 1000);
}

GraphRenderer::~GraphRenderer()
{
  delete image;
}

void GraphRenderer::setSize(const QSize& size)
{
  if(width == size.width() && height == size.height())
    return;
  width = size.width();
  height = size.height();
  upToDate = false;
}

void GraphRenderer::setMaxAge(int maxAge)
{
  if(maxAge == this->maxAge)
    return;
  this->maxAge = maxAge;
  upToDate = false;
}

void GraphRenderer::setEnabledData(unsigned int data)
{
  if(data == this->enabledData)
    return;
  this->enabledData = data;
  upToDate = false;
}

void GraphRenderer::addTradeData(const QList<EDataTradeData::Trade>& data)
{
  qint64 now = data.back().time;

  // add trades to trade handler
  for(QList<EDataTradeData::Trade>::ConstIterator i = data.begin(), end = data.end(); i != end; ++i)
  {
    const EDataTradeData::Trade& trade = *i;

    quint64 time = trade.time / 1000;
    if(tradeSamples.isEmpty() || tradeSamples.last().time != time)
    {
      tradeSamples.append(TradeSample());
      TradeSample& tradeSample =  tradeSamples.last();
      tradeSample.time = time;
      tradeSample.min = tradeSample.max = tradeSample.first = tradeSample.last= trade.price;
      tradeSample.amount = trade.amount;
    }
    else
    {
      TradeSample& tradeSample =  tradeSamples.last();
      tradeSample.last = trade.price;
      if(trade.price < tradeSample.min)
        tradeSample.min = trade.price;
      else if(trade.price > tradeSample.max)
        tradeSample.max = trade.price;
      tradeSample.amount += trade.amount;
    }

    qint64 tradeAge = now - trade.time;
    if(tradeAge < 24ULL * 60ULL * 60ULL * 1000ULL)
    {
      tradeHandler.add(trade, tradeAge);

      if(tradeAge == 0)
      {
        while(!tradeSamples.isEmpty() && time - tradeSamples.front().time > 7ULL * 24ULL * 60ULL * 60ULL)
          tradeSamples.pop_front();

        values = &tradeHandler.values;
      }
    }

    quint64 lowResTime = trade.time / (1000 * 300) * 300;
    if(lowResTradeSamples.isEmpty() || lowResTradeSamples.last().time != lowResTime)
    {
      lowResTradeSamples.append(TradeSample());
      TradeSample& tradeSample = lowResTradeSamples.last();
      tradeSample.time = lowResTime;
      tradeSample.min = tradeSample.max = tradeSample.first = tradeSample.last= trade.price;
      tradeSample.amount = trade.amount;
    }
    else
    {
      TradeSample& tradeSample = lowResTradeSamples.last();
      tradeSample.last = trade.price;
      if(trade.price < tradeSample.min)
        tradeSample.min = trade.price;
      else if(trade.price > tradeSample.max)
        tradeSample.max = trade.price;
      tradeSample.amount += trade.amount;
    }
  }

  // mark graph as dirty
  upToDate = false;
}

void GraphRenderer::addSessionMarker(const EBotSessionMarker& marker)
{
  markers.insert(marker.getDate() / 1000, marker.getType());
  upToDate = false;
}

void GraphRenderer::clearSessionMarker()
{
  markers.clear();
  upToDate = false;
}

QImage& GraphRenderer::render(const QMap<QString, GraphRenderer*>& graphDataByName)
{
  if(!tradeSamples.isEmpty())
  {
    const TradeSample& tradeSample = tradeSamples.back();
    addToMinMax(tradeSample.max);
    addToMinMax(tradeSample.min);
    if(tradeSample.time > ownTime)
      ownTime = tradeSample.time;
    if(ownTime > time)
      time = ownTime;
  }

  if(enabledData & (int)Data::otherMarkets)
  {
    for(QMap<QString, GraphRenderer*>::ConstIterator i = graphDataByName.begin(), end = graphDataByName.end(); i != end; ++i)
    {
      const QList<TradeSample>& tradeSamples = i.value()->tradeSamples;
      if(tradeSamples.isEmpty())
        continue;
      const TradeSample& tradeSample = tradeSamples.back();
      if(tradeSample.time > time)
        time = tradeSample.time;
    }
  }

  for(;;)
  {
    if(!image || image->width() != width || image->height() != height)
    {
      delete image;
      image = new QImage(width, height, QImage::Format_RGB32);
    }
    QPainter painter(image);
    painter.fillRect(0, 0, width, height, QApplication::palette().base());
    painter.setRenderHint(QPainter::Antialiasing);

    double vmin = totalMin;
    double vmax = qMax(totalMax, vmin + 1.);
    const QSize priceSize = painter.fontMetrics().size(Qt::TextSingleLine, formatPrice(totalMax == 0. ? 0. : vmax));

    QRect plotRect(10, 10, width - (10 + priceSize.width() + 8), height - 20 - priceSize.height() + 5);
    if(plotRect.width() <= 0 || plotRect.height() <= 0)
    { // too small
      upToDate = true;
      return *image;
    }

    double lastTotalMin = totalMin;
    double lastTotalMax = totalMax;
    double lastVolumeMax = volumeMax;

    totalMin = DBL_MAX;
    totalMax = 0.;
    volumeMax = 0.;

    if(enabledData & (int)Data::otherMarkets)
    {
      if(values)
      {
        static unsigned int colors[] = { 0x0000FF, 0x8A2BE2, 0xA52A2A, 0x5F9EA0, 0x7FFF00, 0xD2691E, 0xFF7F50, 0x6495ED, 0xDC143C, 0x00FFFF, 0x00008B, 0x008B8B, 0xB8860B, 0xA9A9A9, 0x006400, 0xBDB76B, 0x8B008B, 0x556B2F, 0xFF8C00, 0x9932CC, 0x8B0000, 0xE9967A, 0x8FBC8F, 0x483D8B, 0x2F4F4F, 0x00CED1, 0x9400D3, 0xFF1493, 0x00BFFF, 0x696969, 0x1E90FF, 0xB22222, 0x228B22, 0xFF00FF, 0xFFD700, 0xDAA520, 0x808080, 0x008000, 0xADFF2F, 0xFF69B4, 0xCD5C5C, 0x4B0082 };
        int nextColorIndex = 0;
        double averagePrice = values->regressions[(int)TradeHandler::Regressions::regression24h].average;
        if(averagePrice > 0.)
          for(QMap<QString, GraphRenderer*>::ConstIterator i = graphDataByName.begin(), end = graphDataByName.end(); i != end; ++i)
          {
            const GraphRenderer* channelGraphData = i.value();
            if(channelGraphData != this || !(enabledData & ((int)Data::trades)))
            {
              const TradeHandler::Values* values = channelGraphData->values;
              if(values)
              {
                int colorIndex = nextColorIndex % (sizeof(colors) / sizeof(*colors));
                double otherAveragePrice = values->regressions[(int)TradeHandler::Regressions::regression24h].average;
                if(otherAveragePrice > 0.)
                {
                  QColor color(colors[colorIndex]);
                  color.setAlpha(0x70);
                  prepareTradePolyline(plotRect, vmin, vmax, lastVolumeMax, *channelGraphData, (int)Data::trades, averagePrice / otherAveragePrice, color);
                }
              }
              nextColorIndex++;
            }
          }
      }
    }
    if(enabledData & ((int)Data::trades | (int)Data::tradeVolume) && !tradeSamples.isEmpty())
      prepareTradePolyline(plotRect, vmin, vmax, lastVolumeMax, *this, enabledData, 1., QColor(0, 0, 0));

    if(totalMax == 0.)
    { // no data to draw
      upToDate = true;
      return *image;
    }
    totalMin = floor(totalMin);
    totalMax = ceil(totalMax);
    if(!(totalMin == lastTotalMin && totalMax == lastTotalMax && (!(enabledData & (int)Data::tradeVolume) || qAbs(lastVolumeMax -  volumeMax) <= volumeMax * 0.5)))
      continue;

    drawAxesLables(painter, plotRect, vmin, vmax, priceSize);
    drawTradePolylines(painter);

    if(enabledData & (int)Data::regressionLines)
      drawRegressionLines(painter, plotRect, vmin, vmax);
    if(enabledData & (int)Data::expRegressionLines)
      drawExpRegressionLines(painter, plotRect, vmin, vmax);

    //if(enabledData & (int)Data::sessionMarkers)
      drawMarkers(painter, plotRect, vmin, vmax);

    if(enabledData & (int)Data::key)
      drawKey(painter);
    break;
  }

  upToDate = true;
  return *image;
}

void GraphRenderer::prepareTradePolyline(const QRect& rect, double ymin, double ymax, double lastVolumeMax, const GraphRenderer& graphModel, int enabledData, double scale, const QColor& color)
{
  double yrange = ymax - ymin;
  quint64 vmax = time;
  quint64 vmin = vmax - maxAge;
  quint64 vrange = vmax - vmin;
  int left = rect.left();
  double bottom = rect.bottom();
  int bottomInt = rect.bottom();
  double height = rect.height();
  quint64 width = rect.width();

  PolylineData& graphModelData = this->polylineData[&graphModel];
  unsigned int polyDataSize = rect.width() * 4;
  unsigned int volumeDataSize = rect.width() * 2;
  if(graphModelData.polyDataSize < polyDataSize)
  {
    delete graphModelData.polyData;
    graphModelData.polyData = new QPointF[polyDataSize];
    graphModelData.polyDataSize = polyDataSize;
  }
  if(graphModelData.volumeDataSize < volumeDataSize)
  {
    delete graphModelData.volumeData;
    graphModelData.volumeData = new QPoint[volumeDataSize];
    graphModelData.volumeDataSize = volumeDataSize;
  }
  QPointF* polyData = graphModelData.polyData;
  QPoint* volumeData = graphModelData.volumeData;

  {
    QPointF* currentPoint = polyData;
    QPoint* currentVolumePoint = volumeData;

    const QList<TradeSample>& tradeSamples = maxAge < 7 * 24 * 60 * 60 ? graphModel.tradeSamples : graphModel.lowResTradeSamples;
    int i = 0, count = tradeSamples.size();
    for(int step = qMax(count / 2, 1);;)
    {
      if(i + step < count && tradeSamples.at(i + step).time < vmin)
        i += step;
      else if(step == 1)
        break;
      if(step > 1)
        step /= 2;
    }
    if(i < count)
    {
      const TradeSample* sample = &tradeSamples.at(i);

      int pixelX = 0;
      quint64 currentTimeMax = vmin + vrange / rect.width();
      double currentMin = sample->min;
      double currentMax = sample->max;
      double currentVolume = 0;
      int currentEntryCount = 0;
      double currentLast = 0.;
      double currentFirst = sample->first;
      double lastLast = 0.;
      int lastLeftPixelX = 0;

      for(;;)
      {
        if(sample->time > currentTimeMax)
          goto addLine;

        if(sample->min < currentMin)
          currentMin = sample->min;
        if(sample->max > currentMax)
          currentMax = sample->max;
        currentVolume += sample->amount;
        currentLast = sample->last;
        ++currentEntryCount;

        if(++i >= count)
          goto addLine;
        sample = &tradeSamples.at(i);
        continue;

      addLine:
        if(currentEntryCount > 0)
        {
          if(enabledData & (int)Data::tradeVolume)
          {
            currentVolumePoint->setX(left + pixelX);
            currentVolumePoint->setY(bottomInt);
            ++currentVolumePoint;
            currentVolumePoint->setX(left + pixelX);
            currentVolumePoint->setY(bottomInt - currentVolume * (height * (1. / 3.)) / lastVolumeMax);
            ++currentVolumePoint;

            if(currentVolume > volumeMax)
              volumeMax = currentVolume;
          }

          if(currentMin * scale < totalMin)
            totalMin = currentMin * scale;
          if(currentMax * scale > totalMax)
            totalMax = currentMax * scale;

          if(lastLeftPixelX != 0 && lastLeftPixelX != left + pixelX - 1)
          {
            currentPoint->setX(left + pixelX - 1);
            currentPoint->setY(bottom - (lastLast * scale - ymin) * height / yrange);
            ++currentPoint;
          }
          currentPoint->setX(left + pixelX);
          currentPoint->setY(bottom - (currentFirst * scale - ymin) * height / yrange);
          ++currentPoint;
          if(currentFirst - currentMin < currentFirst - currentMax)
          {
            if(currentMin != currentFirst)
            {
              currentPoint->setX(left + pixelX);
              currentPoint->setY(bottom - (currentMin * scale - ymin) * height / yrange);
              ++currentPoint;
            }
            if(currentMax != currentMin)
            {
              currentPoint->setX(left + pixelX);
              currentPoint->setY(bottom - (currentMax * scale - ymin) * height / yrange);
              ++currentPoint;
            }
            if(currentLast != currentMax)
            {
              currentPoint->setX(left + pixelX);
              currentPoint->setY(bottom - (currentLast * scale - ymin) * height / yrange);
              ++currentPoint;
            }
          }
          else
          {
            if(currentMax != currentFirst)
            {
              currentPoint->setX(left + pixelX);
              currentPoint->setY(bottom - (currentMax * scale - ymin) * height / yrange);
              ++currentPoint;
            }
            if(currentMin != currentMax)
            {
              currentPoint->setX(left + pixelX);
              currentPoint->setY(bottom - (currentMin * scale - ymin) * height / yrange);
              ++currentPoint;
            }
            if(currentLast != currentMin)
            {
              currentPoint->setX(left + pixelX);
              currentPoint->setY(bottom - (currentLast * scale - ymin) * height / yrange);
              ++currentPoint;
            }
          }
          lastLast = currentLast;
          lastLeftPixelX = left + pixelX;
        }

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
    Q_ASSERT(currentVolumePoint - volumeData <= rect.width() * 2);

    graphModelData.updated = true;
    graphModelData.color = color;
    graphModelData.polyDataCount = currentPoint - polyData;
    graphModelData.volumeDataCount = (currentVolumePoint - volumeData) / 2;
  }
}

void GraphRenderer::drawAxesLables(QPainter& painter, const QRect& rect, double vmin, double vmax, const QSize& priceSize)
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

    double vstart = ceil(vmin / vstep) * vstep - vmin;

    for(int i = 0;; ++i)
    {
      QPoint right(rect.right() + 4, rect.bottom() - (vstart + vstep * i) * height / vrange);

      if(right.y() < rect.top())
        break;

      painter.setPen(linePen);
      painter.drawLine(QPoint(0, right.y()), right);
      painter.setPen(textPen);
      QPoint textPos(right.x() + 2, right.y() + priceSize.height() * 0.5 - 3);
      if(textPos.y() > 2)
        painter.drawText(textPos, formatPrice(vmin + vstart + i * vstep));
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

void GraphRenderer::drawTradePolylines(QPainter& painter)
{
  bool cleanup = false;
  PolylineData* focusGraphModelData = 0;
  for(QHash<const GraphRenderer*, PolylineData>::Iterator i = polylineData.begin(), end = polylineData.end(); i != end; ++i)
  {
    PolylineData& graphModelData = *i;
    if(!graphModelData.updated)
    {
      graphModelData.removeThis = true;
      cleanup = true;
      continue;
    }
    if(!graphModelData.polyDataCount)
      continue;
    const GraphRenderer* graphData = i.key();
    if(graphData == this)
      focusGraphModelData = &graphModelData;
    else
    {
      QPen rangePen(graphModelData.color);
      rangePen.setWidth(2);
      painter.setPen(rangePen);
      painter.drawPolyline(graphModelData.polyData, graphModelData.polyDataCount);
    }
    graphModelData.updated = false;
  }
  if(focusGraphModelData)
  {
    if(focusGraphModelData->volumeDataCount > 0)
    {
      QPen volumePen(Qt::darkBlue);
      painter.setPen(volumePen);
      painter.drawLines(focusGraphModelData->volumeData, focusGraphModelData->volumeDataCount);
    }
    if(focusGraphModelData->polyDataCount > 0)
    {
      QPen rangePen(focusGraphModelData->color);
      rangePen.setWidth(2);
      painter.setPen(rangePen);
      painter.drawPolyline(focusGraphModelData->polyData, focusGraphModelData->polyDataCount);
    }
  }
  if(cleanup)
  {
    for(QHash<const GraphRenderer*, PolylineData>::Iterator i = polylineData.begin(), end = polylineData.end(); i != end; ++i)
    {
      const PolylineData& graphModelData = *i;
      if(graphModelData.removeThis)
      {
        this->polylineData.erase(i);
        break;
      }
    }
  }
}

void GraphRenderer::drawRegressionLines(QPainter& painter, const QRect& rect, double vmin, double vmax)
{
  if(!values)
    return;

  double vrange = vmax - vmin;
  quint64 hmax = time;
  quint64 hmin = hmax - maxAge;
  quint64 hrange = hmax - hmin;
  quint64 width = rect.width();
  double height = rect.height();

  static quint64 depths[] = {1 * 60, 3 * 60, 5 * 60, 10 * 60, 15 * 60, 20 * 60, 30 * 60, 1 * 60 * 60, 2 * 60 * 60, 4 * 60 * 60, 6 * 60 * 60, 12 * 60 * 60, 24 * 60 * 60};
  for(int i = 0; i < (int)TradeHandler::Regressions::numOfRegressions; ++i)
  {
    const TradeHandler::Values::RegressionLine& rl = values->regressions[i];

    const quint64& endTime = ownTime;
    quint64 startTime = qMax(endTime - depths[i], hmin);
    if((qint64)hmin - (qint64)startTime > maxAge / 2)
      break;
    
    double val = rl.price - rl.incline * (endTime - startTime);
    QPointF a(rect.left() + (startTime - hmin) * width / hrange, rect.bottom() - (val -  vmin) * height / vrange);
    QPointF b(rect.right() - (time - endTime) * width / hrange, rect.bottom() - (rl.price -  vmin) * height / vrange);

    int color = qMin((int)((0xdd - 0x64) * qMin(qMax(fabs(rl.incline / 0.005), 0.), 1.)), 0xdd - 0x64);
    QPen pen(rl.incline >= 0 ? QColor(0, color + 0x64, 0) : QColor(color + 0x64, 0, 0));
    pen.setWidth(2);
    painter.setPen(pen);

    painter.drawLine(a, b);
  }
}

void GraphRenderer::drawExpRegressionLines(QPainter& painter, const QRect& rect, double vmin, double vmax)
{
  if(!values)
    return;

  double vrange = vmax - vmin;
  quint64 hmax = time;
  quint64 hmin = hmax - maxAge;
  quint64 hrange = hmax - hmin;
  quint64 width = rect.width();
  double height = rect.height();

  static quint64 depths[] = {1 * 60, 3 * 60, 5 * 60, 10 * 60, 15 * 60, 20 * 60, 30 * 60, 1 * 60 * 60, 2 * 60 * 60, 4 * 60 * 60, 6 * 60 * 60, 12 * 60 * 60, 24 * 60 * 60};
  for(int i = 0; i < (int)TradeHandler::BellRegressions::numOfBellRegressions; ++i)
  {
    const TradeHandler::Values::RegressionLine& rl = values->bellRegressions[i];

    const quint64& endTime = ownTime;
    quint64 startTime = qMax(endTime - depths[i] * 3, hmin);
    if((qint64)hmin - (qint64)startTime > maxAge / 2)
      break;

    double val = rl.price - rl.incline * (endTime - startTime);
    QPointF a(rect.left() + (startTime - hmin) * width / hrange, rect.bottom() - (val -  vmin) * height / vrange);
    QPointF b(rect.right() - (time - endTime) * width / hrange, rect.bottom() - (rl.price -  vmin) * height / vrange);

    int color = qMin((int)((0xdd - 0x64) * qMin(qMax(fabs(rl.incline / 0.005), 0.), 1.)), 0xdd - 0x64);
    QPen pen(rl.incline >= 0 ? QColor(0, color + 0x64, 0) : QColor(color + 0x64, 0, 0));
    pen.setWidth(2);
    painter.setPen(pen);

    painter.drawLine(a, b);
  }
}

void GraphRenderer::drawMarkers(QPainter& painter, const QRect& rect, double vmin, double vmax)
{
  int leftInt = rect.left();
  int widthInt = rect.width();
  int bottomInt = rect.bottom();
  int topInt = rect.top();
  int midInt = (rect.bottom() + rect.top()) / 2;
  int bottomMidInt = (rect.bottom() + midInt) / 2;
  int topMidInt = (rect.top() + midInt) / 2;
  quint64 hmax = time;
  quint64 hmin = hmax - maxAge;

  QPoint* cyanLineData = (QPoint*)alloca(markers.size() * 2 * sizeof(QPoint));
  QPoint* orangeLineData = (QPoint*)alloca(markers.size() * 2 * sizeof(QPoint));
  QPoint* cyanCurrentLinePoint = cyanLineData;
  QPoint* orangeCurrentLinePoint = orangeLineData;
  for(QMap<quint64, EBotSessionMarker::Type>::ConstIterator i = markers.begin(), end = markers.end(); i != end; ++i)
  {
    const quint64& time = i.key();
    if(time < hmin)
      continue;
    int x = leftInt + (time - hmin) * widthInt / maxAge;
    EBotSessionMarker::Type markerType = i.value();
    QPoint* currentLinePoint;
    if(markerType >= EBotSessionMarker::Type::goodBuy)
    {
      currentLinePoint = orangeCurrentLinePoint;
      orangeCurrentLinePoint += 2;
    }
    else
    {
      currentLinePoint = cyanCurrentLinePoint;
      cyanCurrentLinePoint += 2;
    }
    currentLinePoint[0].setX(x);
    currentLinePoint[1].setX(x);
    switch(i.value())
    {
    case EBotSessionMarker::Type::buy:
    case EBotSessionMarker::Type::goodBuy:
      currentLinePoint[0].setY(bottomInt);
      currentLinePoint[1].setY(midInt);
      break;
    case EBotSessionMarker::Type::buyAttempt:
      currentLinePoint[0].setY(bottomInt);
      currentLinePoint[1].setY(bottomMidInt);
      break;
    case EBotSessionMarker::Type::sell:
    case EBotSessionMarker::Type::goodSell:
      currentLinePoint[0].setY(midInt);
      currentLinePoint[1].setY(topInt);
      break;
    case EBotSessionMarker::Type::sellAttempt:
      currentLinePoint[0].setY(topMidInt);
      currentLinePoint[1].setY(topInt);
      break;
    }
    currentLinePoint += 2;
  }

  if(cyanCurrentLinePoint > cyanLineData)
  {
    painter.setPen(QPen(Qt::darkCyan));
    painter.drawLines(cyanLineData, (cyanCurrentLinePoint - cyanLineData) / 2);
  }
  if(orangeCurrentLinePoint > orangeLineData)
  {
    painter.setPen(QPen(Qt::darkYellow));
    painter.drawLines(orangeLineData, (orangeCurrentLinePoint - orangeLineData) / 2);
  }
}

void GraphRenderer::drawKey(QPainter& painter)
{
  QSize maxSize;
  int& rheight = maxSize.rheight();
  int& rwidth = maxSize.rwidth();
  rwidth = 0;
  int polylineCount = 0;
  const PolylineData* focusGraphModelData = 0;
  for(QHash<const GraphRenderer*, PolylineData>::Iterator i = polylineData.begin(), end = polylineData.end(); i != end; ++i)
  {
    PolylineData& data = *i;
    if(data.removeThis || !data.polyDataCount)
      continue;

    const GraphRenderer* graphData = i.key();
    const QSize labelSize = painter.fontMetrics().size(Qt::TextSingleLine, graphData->getChannelName());
    rheight = labelSize.height();
    int width = labelSize.width();
    if(width > rwidth)
      rwidth = width;
    if(graphData == this)
      focusGraphModelData = &data;
    ++polylineCount;
  }

  const QPalette& platte = QApplication::palette();
  QColor bgColor = platte.base().color();
  bgColor.setAlpha(0xaa);
  painter.setBrush(bgColor);
  painter.setPen(Qt::darkGray);
  painter.drawRect(10, 10, rwidth + 20 + 20, (rheight + 10) * polylineCount + 10);
  int ypos = 20;
  painter.setPen(platte.text().color());
  
  if(focusGraphModelData)
  {
    painter.fillRect(QRect(20, ypos + (rheight - 10) / 2 + 1, 10, 10), focusGraphModelData->color);
    painter.drawText(QRect(20 + 20, ypos, rwidth, rheight), getChannelName());
    ypos += rheight + 10;
  }

  for(QHash<const GraphRenderer*, PolylineData>::Iterator i = polylineData.begin(), end = polylineData.end(); i != end; ++i)
  {
    PolylineData& data = *i;

    if(data.removeThis || !data.polyDataCount)
      continue;

    if(&data != focusGraphModelData)
    {
      painter.fillRect(QRect(20, ypos + (rheight - 10) / 2 + 1, 10, 10), data.color);
      painter.drawText(QRect(20 + 20, ypos, rwidth, rheight), i.key()->getChannelName());
      ypos += rheight + 10;
    }
  }
}
