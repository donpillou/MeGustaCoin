
#include "stdafx.h"

BotOrderWidget::BotOrderWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager) :
  QWidget(&tabFramework), tabFramework(tabFramework), entityManager(entityManager),  orderModel(entityManager)
{
  setWindowTitle(tr("Bot Orders"));

  orderView = new QTreeView(this);
  orderView->setUniformRowHeights(true);
  SessionOrderSortProxyModel* orderProxyModel = new SessionOrderSortProxyModel(this);
  orderProxyModel->setDynamicSortFilter(true);
  orderProxyModel->setSourceModel(&orderModel);
  orderView->setModel(orderProxyModel);
  orderView->setSortingEnabled(true);
  orderView->setRootIsDecorated(false);
  orderView->setAlternatingRowColors(true);
  //orderView->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::DoubleClicked);
  //orderView->setSelectionMode(QAbstractItemView::ExtendedSelection);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->addWidget(orderView);
  setLayout(layout);

  QHeaderView* headerView = orderView->header();
  headerView->resizeSection(0, 50);
  headerView->resizeSection(1, 110);
  headerView->resizeSection(2, 85);
  headerView->resizeSection(3, 100);
  headerView->resizeSection(4, 85);
  headerView->resizeSection(5, 85);
  headerView->setStretchLastSection(false);
  headerView->setResizeMode(0, QHeaderView::Stretch);
  orderView->sortByColumn(1);
  settings.beginGroup("BotOrders");
  headerView->restoreState(settings.value("HeaderState").toByteArray());
  settings.endGroup();
}


void BotOrderWidget::saveState(QSettings& settings)
{
  settings.beginGroup("BotOrders");
  settings.setValue("HeaderState", orderView->header()->saveState());
  settings.endGroup();
}
