
#include "stdafx.h"

TradesWidget::TradesWidget(QWidget* parent, QSettings& settings, DataModel& dataModel) : QWidget(parent), dataModel(dataModel), tradeModel(dataModel.tradeModel), autoScrollEnabled(false)
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
  headerView->restoreState(settings.value("TradeHeaderState").toByteArray());
}

void TradesWidget::saveState(QSettings& settings)
{
  settings.setValue("TradeHeaderState", tradeView->header()->saveState());
}

void TradesWidget::setMarket(Market* market)
{
  this->market = market;
}

void TradesWidget::checkAutoScroll(const QModelIndex& index, int, int)
{
  QScrollBar* scrollBar = tradeView->verticalScrollBar();
  autoScrollEnabled = scrollBar->value() == scrollBar->maximum();
  QTimer::singleShot(100, this, SLOT(clearAbove()));
}

void TradesWidget::autoScroll(int, int)
{
  if(!autoScrollEnabled)
    return;
  QScrollBar* scrollBar = tradeView->verticalScrollBar();
  scrollBar->setValue(scrollBar->maximum());
}

void TradesWidget::clearAbove()
{
  tradeModel.clearAbove(500);
}
