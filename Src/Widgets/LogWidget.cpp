
#include "stdafx.h"

LogWidget::LogWidget(QWidget* parent, QSettings& settings, LogModel& logModel) : QWidget(parent), logModel(logModel), autoScrollEnabled(true)
{
  connect(&logModel, SIGNAL(rowsAboutToBeInserted(const QModelIndex&, int, int)), this, SLOT(checkAutoScroll(const QModelIndex&, int, int)));

  logView = new QTreeView(this);
  connect(logView->verticalScrollBar(), SIGNAL(rangeChanged(int, int)), this, SLOT(autoScroll(int, int)));
  logView->setModel(&logModel);
  //logView->setSortingEnabled(true);
  logView->setRootIsDecorated(false);
  logView->setAlternatingRowColors(true);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->addWidget(logView);
  setLayout(layout);

  QHeaderView* headerView = logView->header();
  headerView->resizeSection(0, 110);
  headerView->resizeSection(1, 200);
  logView->sortByColumn(0, Qt::AscendingOrder);
  headerView->restoreState(settings.value("LogHeaderState").toByteArray());
}

void LogWidget::saveState(QSettings& settings)
{
  settings.setValue("LogHeaderState", logView->header()->saveState());
}

void LogWidget::checkAutoScroll(const QModelIndex& index, int, int)
{
  QScrollBar* scrollBar = logView->verticalScrollBar();
  autoScrollEnabled = scrollBar->value() == scrollBar->maximum();
}

void LogWidget::autoScroll(int, int)
{
  if(!autoScrollEnabled)
    return;
  QScrollBar* scrollBar = logView->verticalScrollBar();
  scrollBar->setValue(scrollBar->maximum());
}
