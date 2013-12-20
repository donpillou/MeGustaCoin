
#include "stdafx.h"

GraphWidget::GraphWidget(QWidget* parent, QSettings& settings, DataModel& dataModel) : QFrame(parent), dataModel(dataModel), graphModel(dataModel.graphModel)
{
  setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
  setBackgroundRole(QPalette::Base);
  setAutoFillBackground(true);
  /*
  QToolBar* toolBar = new QToolBar(this);
  toolBar->setStyleSheet("QToolBar { border: 0px }");
  toolBar->setIconSize(QSize(16, 16));
  toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  QAction* action = toolBar->addAction(QIcon(":/Icons/arrow_refresh.png"), tr("&Refresh"));
  action->setEnabled(false);
  action->setShortcut(QKeySequence(QKeySequence::Refresh));
  connect(action, SIGNAL(triggered()), this, SLOT(refresh()));

  toolBar->move(QPoint(0, 0));
  toolBar->resize(QSize(100, 100));
  */


  connect(&graphModel, SIGNAL(dataAdded()), this, SLOT(update()));
}

void GraphWidget::saveState(QSettings& settings)
{
}

void GraphWidget::setMarket(Market* market)
{
  this->market = market;
}

void GraphWidget::paintEvent(QPaintEvent* event)
{
  QFrame::paintEvent(event);

  QRect rect = this->rect();
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setPen(Qt::black);

  if (graphModel.trades.size() <= 0)
    return; // no data

  double hmin = floor(graphModel.totalMin);
  double hmax = -floor(-qMax(graphModel.totalMax, hmin + 1.));
  const QSize priceSize = painter.fontMetrics().size(Qt::TextSingleLine, market->formatPrice(hmax));

  QRect plotRect(10, 12, rect.width() - (10 + priceSize.width() + 8), rect.height() - 22);
  if (plotRect.width() <= 0 || plotRect.height() <= 0)
    return; // too small

  //drawBox(painter, plotRect);
  drawAxisLables(painter, plotRect, hmin, hmax, priceSize);
  drawTradePolyline(painter, plotRect, hmin, hmax);
}

void GraphWidget::drawBox(QPainter& painter, const QRect& rect)
{
  painter.drawRect(rect);
}

void GraphWidget::drawAxisLables(QPainter& painter, const QRect& rect, double& hmin, double& hmax, const QSize& priceSize)
{
  double hrange = hmax - hmin;
  double height = rect.height();
  double hstep = pow(10., ceil(log10(hrange * 20. / height)));
  if(hstep * height / hrange >= 40.f)
    hstep *= 0.5;
  if(hstep * height / hrange >= 40.f)
    hstep *= 0.5;
  /*
  hmax += hstep * 1.5;
  hrange = hmax - hmin;
  
  hstep = pow(10., ceil(log10(hrange * 20. / height)));
  if(hstep * height / hrange >= 40.f)
    hstep *= 0.5;
  if(hstep * height / hrange >= 40.f)
    hstep *= 0.5;
    */

  /*
  int pixelHStep = hstep * height / hrange;
  int steps = rect.height() / pixelHStep;
  int bottomOffset = rect.height() - set * pixelHStep;
    */

  //painter.drawText(QPointF(rect.right() + 5, rect.top() + priceSize.height() * 0.5 - 2), market->formatPrice(hmax));
  //painter.drawText(QPointF(rect.right() + 5, rect.bottom() + priceSize.height() * 0.5 - 2), market->formatPrice(hmin));

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
void GraphWidget::drawTradePolyline(QPainter& painter, const QRect& rect, double hmin, double hmax)
{
  double hrange = hmax - hmin;
  quint64 vmin = graphModel.trades.begin().key();
  quint64 vmax = (--graphModel.trades.end()).key();
  quint64 vrange = vmax - vmin;
  int left = rect.left();
  double bottom = rect.bottom();
  double height = rect.height();
  quint64 width = rect.width();
  // if(vrange > xx) // todo

  painter.setPen(Qt::black);

  QPointF* polyData = (QPointF*)alloca(rect.width() * sizeof(QPointF));
  {
    int pixelX = 0;
    QPointF* currentPoint = polyData;
    quint64 currentTimeMax = vmin + vrange / rect.width();
    double currentMin = graphModel.trades.begin()->min;
    int currentEntryCount = 0;
    double currentLast = 0.;

    foreach(const GraphModel::Entry& entry, graphModel.trades)
    {
      while(entry.time > currentTimeMax)
      {
        Q_ASSERT(currentPoint - polyData < rect.width());
        currentPoint->setX(left + pixelX);
        currentPoint->setY(bottom - (currentMin - hmin) * height / hrange);
        if(currentEntryCount > 0)
          ++currentPoint;
        else if(currentLast != 0.)
        {
          currentPoint->setY(bottom - (currentLast - hmin) * height / hrange);
          ++currentPoint;
        }
        ++pixelX;
        currentTimeMax = vmin + vrange * (pixelX + 1) / width;
        currentMin = entry.min;
        currentEntryCount = 0;
      }
      if(entry.min < currentMin)
        currentMin = entry.min;
      currentLast = entry.min;
      ++currentEntryCount;
    }
    Q_ASSERT(currentPoint - polyData < rect.width());
    currentPoint->setX(left + pixelX);
    currentPoint->setY(bottom - (currentMin - hmin) * height / hrange);
    if(currentEntryCount > 0)
      ++currentPoint;
    else if(currentLast != 0.)
    {
      currentPoint->setY(bottom - (currentLast - hmin) * height / hrange);
      ++currentPoint;
    }

    painter.drawPolyline(polyData, currentPoint - polyData);
  }
  {
    int pixelX = 0;
    QPointF* currentPoint = polyData;
    quint64 currentTimeMax = vmin + vrange / rect.width();
    double currentMax = graphModel.trades.begin()->max;
    int currentEntryCount = 0;
    double currentLast = 0.;

    foreach(const GraphModel::Entry& entry, graphModel.trades)
    {
      while(entry.time > currentTimeMax)
      {
        Q_ASSERT(currentPoint - polyData < rect.width());
        currentPoint->setX(left + pixelX);
        currentPoint->setY(bottom - (currentMax - hmin) * height / hrange);
        if(currentEntryCount > 0)
          ++currentPoint;
        else if(currentLast != 0.)
        {
          currentPoint->setY(bottom - (currentLast - hmin) * height / hrange);
          ++currentPoint;
        }
        ++pixelX;
        currentTimeMax = vmin + vrange * (pixelX + 1) / width;
        currentMax = entry.max;
        currentEntryCount = 0;
      }
      if(entry.max > currentMax)
        currentMax = entry.max;
      currentLast = entry.max;
      ++currentEntryCount;
    }
    Q_ASSERT(currentPoint - polyData < rect.width());
    currentPoint->setX(left + pixelX);
    currentPoint->setY(bottom - (currentMax - hmin) * height / hrange);
    if(currentEntryCount > 0)
      ++currentPoint;
    else if(currentLast != 0.)
    {
      currentPoint->setY(bottom - (currentLast - hmin) * height / hrange);
      ++currentPoint;
    }

    painter.drawPolyline(polyData, currentPoint - polyData);
  }
}
