
#include "stdafx.h"

LogWidget::LogWidget(QWidget* parent, QSettings& settings, Entity::Manager& entityManager) : QWidget(parent),
  logModel(entityManager), autoScrollEnabled(true)
{
  connect(&logModel, SIGNAL(rowsAboutToBeInserted(const QModelIndex&, int, int)), this, SLOT(checkAutoScroll(const QModelIndex&, int, int)));

  setWindowTitle(tr("Log"));

  logView = new QTreeView(this);
  connect(logView->verticalScrollBar(), SIGNAL(rangeChanged(int, int)), this, SLOT(autoScroll(int, int)));
  logView->setUniformRowHeights(true);
  logView->setModel(&logModel);
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
  settings.beginGroup("Log");
  headerView->restoreState(settings.value("HeaderState").toByteArray());
  settings.endGroup();
}

void LogWidget::saveState(QSettings& settings)
{
  settings.beginGroup("Log");
  settings.setValue("HeaderState", logView->header()->saveState());
  settings.endGroup();
}

void LogWidget::checkAutoScroll(const QModelIndex& index, int, int)
{
  QScrollBar* scrollBar = logView->verticalScrollBar();
  if(scrollBar->value() == scrollBar->maximum())
    autoScrollEnabled = true;
}

void LogWidget::autoScroll(int, int)
{
  if(!autoScrollEnabled)
    return;
  QScrollBar* scrollBar = logView->verticalScrollBar();
  scrollBar->setValue(scrollBar->maximum());
  autoScrollEnabled = false;
}
