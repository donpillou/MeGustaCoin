
#include "stdafx.h"

BotPropertiesWidget::BotPropertiesWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager) :
  QWidget(&tabFramework), tabFramework(tabFramework), entityManager(entityManager),  propertyModel(entityManager)
{
  setWindowTitle(tr("Bot Properties"));

  propertyView = new QTreeView(this);
  propertyView->setUniformRowHeights(true);
  QSortFilterProxyModel* propertyProxyModel = new QSortFilterProxyModel(this);
  propertyProxyModel->setDynamicSortFilter(true);
  propertyProxyModel->setSourceModel(&propertyModel);
  propertyView->setModel(propertyProxyModel);
  propertyView->setSortingEnabled(true);
  propertyView->setRootIsDecorated(false);
  propertyView->setAlternatingRowColors(true);
  //propertyView->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::DoubleClicked);
  //propertyView->setSelectionMode(QAbstractItemView::ExtendedSelection);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->addWidget(propertyView);
  setLayout(layout);

  QHeaderView* headerView = propertyView->header();
  //propertyView->sortByColumn(0);
  settings.beginGroup("BotProperties");
  headerView->restoreState(settings.value("HeaderState").toByteArray());
  settings.endGroup();
  //headerView->setStretchLastSection(false);
  //headerView->setResizeMode(0, QHeaderView::Stretch);
}


void BotPropertiesWidget::saveState(QSettings& settings)
{
  settings.beginGroup("BotProperties");
  settings.setValue("HeaderState", propertyView->header()->saveState());
  settings.endGroup();
}
