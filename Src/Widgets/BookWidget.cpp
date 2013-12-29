
#include "stdafx.h"

BookWidget::BookWidget(QWidget* parent, QSettings& settings, PublicDataModel& publicDataModel) :
  QWidget(parent),
  publicDataModel(publicDataModel), bookModel(publicDataModel.bookModel),
  askAutoScrollEnabled(false), bidAutoScrollEnabled(false)
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

  class MyLayout : public QHBoxLayout
  {
  public:
    MyLayout(QTreeView* bidView, QTreeView* askView) : bidView(bidView), askView(askView)
    {
      setMargin(0);
      setSpacing(0);
      addWidget(bidView);
      addWidget(askView);
    }
    virtual QSize sizeHint() const {return bidView->sizeHint();}
  private:
    QTreeView* bidView;
    QTreeView* askView;
  };

  QHBoxLayout* layout = new MyLayout(bidView, askView);
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

void BookWidget::checkAutoScroll(const QModelIndex& index, int, int)
{
  QScrollBar* askScrollBar = askView->verticalScrollBar();
  QScrollBar* bidScrollBar = bidView->verticalScrollBar();
  if(askScrollBar->value() == askScrollBar->maximum())
    askAutoScrollEnabled = true;
  if(bidScrollBar->value() == bidScrollBar->maximum())
    bidAutoScrollEnabled = true;
}

void BookWidget::autoScroll(int, int)
{
  if(askAutoScrollEnabled)
  {
    QScrollBar* scrollBar = askView->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
    askAutoScrollEnabled = false;
  }
  if(bidAutoScrollEnabled)
  {
    QScrollBar* scrollBar = bidView->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
    bidAutoScrollEnabled = false;
  }
}
