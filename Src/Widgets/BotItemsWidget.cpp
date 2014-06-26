
#include "stdafx.h"

BotItemsWidget::BotItemsWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager) :
  QWidget(&tabFramework), tabFramework(tabFramework), entityManager(entityManager),  itemModel(entityManager)
{
  setWindowTitle(tr("Bot Items"));

  itemView = new QTreeView(this);
  itemView->setUniformRowHeights(true);
  SessionItemSortProxyModel* itemProxyModel = new SessionItemSortProxyModel(this);
  itemProxyModel->setDynamicSortFilter(true);
  itemProxyModel->setSourceModel(&itemModel);
  itemView->setModel(itemProxyModel);
  itemView->setSortingEnabled(true);
  itemView->setRootIsDecorated(false);
  itemView->setAlternatingRowColors(true);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->addWidget(itemView);
  setLayout(layout);

  QHeaderView* headerView = itemView->header();
  headerView->resizeSection(0, 50);
  headerView->resizeSection(1, 50);
  headerView->resizeSection(2, 110);
  headerView->resizeSection(3, 85);
  headerView->resizeSection(4, 100);
  headerView->resizeSection(5, 85);
  headerView->resizeSection(6, 85);
  headerView->resizeSection(7, 85);
  headerView->setStretchLastSection(false);
  headerView->setResizeMode(0, QHeaderView::Stretch);
  itemView->sortByColumn(1);
  settings.beginGroup("BotItems");
  headerView->restoreState(settings.value("HeaderState").toByteArray());
  settings.endGroup();
}


void BotItemsWidget::saveState(QSettings& settings)
{
  settings.beginGroup("BotItems");
  settings.setValue("HeaderState", itemView->header()->saveState());
  settings.endGroup();
}
