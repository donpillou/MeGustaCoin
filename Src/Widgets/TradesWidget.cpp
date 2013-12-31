
#include "stdafx.h"

TradesWidget::TradesWidget(QWidget* parent, QSettings& settings, PublicDataModel& publicDataModel) :
  QWidget(parent),
  publicDataModel(publicDataModel), tradeModel(publicDataModel.tradeModel), autoScrollEnabled(false)
{
  connect(&tradeModel, SIGNAL(rowsAboutToBeInserted(const QModelIndex&, int, int)), this, SLOT(checkAutoScroll(const QModelIndex&, int, int)));

  tradeView = new QTreeView(this);
  connect(tradeView->verticalScrollBar(), SIGNAL(rangeChanged(int, int)), this, SLOT(autoScroll(int, int)));
  tradeView->setModel(&tradeModel);
  //tradeView->setSortingEnabled(true);
  tradeView->setRootIsDecorated(false);
  tradeView->setAlternatingRowColors(true);
  //tradeView->setSelectionMode(QAbstractItemView::ExtendedSelection);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->addWidget(tradeView);
  setLayout(layout);

  QHeaderView* headerView = tradeView->header();
  headerView->resizeSection(0, 110);
  headerView->resizeSection(1, 105);
  headerView->resizeSection(2, 105);
  headerView->setStretchLastSection(false);
  headerView->setResizeMode(1, QHeaderView::Stretch);
  settings.beginGroup("LiveTrades");
  settings.beginGroup(publicDataModel.getMarketName());
  headerView->restoreState(settings.value("HeaderState").toByteArray());
  settings.endGroup();
  settings.endGroup();
}

void TradesWidget::saveState(QSettings& settings)
{
  settings.beginGroup("LiveTrades");
  settings.beginGroup(publicDataModel.getMarketName());
  settings.setValue("HeaderState", tradeView->header()->saveState());
  settings.endGroup();
  settings.endGroup();
}

void TradesWidget::checkAutoScroll(const QModelIndex& index, int, int)
{
  QScrollBar* scrollBar = tradeView->verticalScrollBar();
  if(scrollBar->value() == scrollBar->maximum())
    autoScrollEnabled = true;
  QTimer::singleShot(100, this, SLOT(clearAbove()));
}

void TradesWidget::autoScroll(int, int)
{
  if(!autoScrollEnabled)
    return;
  QScrollBar* scrollBar = tradeView->verticalScrollBar();
  scrollBar->setValue(scrollBar->maximum());
  autoScrollEnabled = false;
}

void TradesWidget::clearAbove()
{
  QScrollBar* scrollBar = tradeView->verticalScrollBar();
  if(scrollBar->value() == scrollBar->maximum())
    autoScrollEnabled = true;
  tradeModel.clearAbove(500);
}
