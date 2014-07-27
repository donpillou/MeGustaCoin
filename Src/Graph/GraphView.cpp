
#include "stdafx.h"
#include <cfloat>

GraphView::GraphView(QWidget* parent, GraphModel& graphModel) :
  QWidget(parent), graphModel(graphModel)
{
  connect(&graphModel, SIGNAL(dataChanged()), this, SLOT(update()));
}

QSize GraphView::sizeHint() const
{
  return QSize(400, 300);
}

//void GraphView::setMaxAge(int maxAge)
//{
//  this->maxAge = maxAge;
//}
//
//void GraphView::setEnabledData(unsigned int data)
//{
//  this->enabledData = data;
//}

void GraphView::paintEvent(QPaintEvent* event)
{
  QPainter painter(this);
  painter.drawImage(0, 0, graphModel.getImage());
}

void GraphView::resizeEvent(QResizeEvent* event)
{
  graphModel.setSize(event->size());
}
