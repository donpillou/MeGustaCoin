
#include "stdafx.h"

BookWidget::BookWidget(QWidget* parent, QSettings& settings, DataModel& dataModel) : QWidget(parent), dataModel(dataModel), bookModel(dataModel.bookModel), market(0), askAutoScrollEnabled(false), bidAutoScrollEnabled(false)
{
  connect(&bookModel.askModel, SIGNAL(rowsAboutToBeInserted(const QModelIndex&, int, int)), this, SLOT(checkAutoScroll(const QModelIndex&, int, int)));
  connect(&bookModel.bidModel, SIGNAL(rowsAboutToBeInserted(const QModelIndex&, int, int)), this, SLOT(checkAutoScroll(const QModelIndex&, int, int)));

  askView = new QTreeView(this);
  bidView = new QTreeView(this);
  connect(askView->verticalScrollBar(), SIGNAL(rangeChanged(int, int)), this, SLOT(autoScroll(int, int)));
  connect(bidView->verticalScrollBar(), SIGNAL(rangeChanged(int, int)), this, SLOT(autoScroll(int, int)));
  askView->setModel(&bookModel.askModel);
  bidView->setModel(&bookModel.bidModel);
  askView->setRootIsDecorated(false);
  bidView->setRootIsDecorated(false);
  askView->setAlternatingRowColors(true);
  bidView->setAlternatingRowColors(true);

  QHBoxLayout* layout = new QHBoxLayout;
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->addWidget(bidView);
  layout->addWidget(askView);
  setLayout(layout);

  QHeaderView* askHeaderView = askView->header();
  QHeaderView* bidHeaderView = bidView->header();
  askHeaderView->resizeSection(0, 105);
  bidHeaderView->resizeSection(0, 105);
  askHeaderView->resizeSection(1, 105);
  bidHeaderView->resizeSection(1, 105);
  askHeaderView->setStretchLastSection(false);
  bidHeaderView->setStretchLastSection(false);
  askHeaderView->setResizeMode(0, QHeaderView::Stretch);
  bidHeaderView->setResizeMode(0, QHeaderView::Stretch);
  askHeaderView->restoreState(settings.value("BookAskHeaderState").toByteArray());
  bidHeaderView->restoreState(settings.value("BookBidHeaderState").toByteArray());
}

void BookWidget::saveState(QSettings& settings)
{
  settings.setValue("BookAskHeaderState", askView->header()->saveState());
  settings.setValue("BookBidHeaderState", bidView->header()->saveState());
}

void BookWidget::setMarket(Market* market)
{
  this->market = market;
}

void BookWidget::checkAutoScroll(const QModelIndex& index, int, int)
{
  QScrollBar* askScrollBar = askView->verticalScrollBar();
  QScrollBar* bidScrollBar = bidView->verticalScrollBar();
  askAutoScrollEnabled = askScrollBar->value() == askScrollBar->maximum();
  bidAutoScrollEnabled = bidScrollBar->value() == bidScrollBar->maximum();
}

void BookWidget::autoScroll(int, int)
{
  if(askAutoScrollEnabled)
  {
    QScrollBar* scrollBar = askView->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
  }
  if(bidAutoScrollEnabled)
  {
    QScrollBar* scrollBar = bidView->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
  }
}
