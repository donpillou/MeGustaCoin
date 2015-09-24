
#include "stdafx.h"

ProcessesWidget::ProcessesWidget(QWidget* parent, QSettings& settings, Entity::Manager& entityManager) : QWidget(parent),
  processModel(entityManager)
{
  setWindowTitle(tr("Processes"));

  processView = new QTreeView(this);
  processView->setUniformRowHeights(true);
  QSortFilterProxyModel* proxyModel = new QSortFilterProxyModel(this);
  proxyModel->setDynamicSortFilter(true);
  proxyModel->setSourceModel(&processModel);
  processView->setSortingEnabled(true);
  processView->setModel(proxyModel);
  processView->setRootIsDecorated(false);
  processView->setAlternatingRowColors(true);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->addWidget(processView);
  setLayout(layout);

  QHeaderView* headerView = processView->header();
  processView->sortByColumn(1);
  settings.beginGroup("Processes");
  headerView->restoreState(settings.value("HeaderState").toByteArray());
  settings.endGroup();
}

void ProcessesWidget::saveState(QSettings& settings)
{
  settings.beginGroup("Processes");
  settings.setValue("HeaderState", processView->header()->saveState());
  settings.endGroup();
}
