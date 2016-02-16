
#include "stdafx.h"

UserSessionOrdersWidget::UserSessionOrdersWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager) :
  QWidget(&tabFramework), ordersModel(entityManager)
{
  setWindowTitle(tr("Bot Orders"));

  orderView = new QTreeView(this);
  orderView->setUniformRowHeights(true);
  UserSessionOrderSortProxyModel* ordersProxyModel = new UserSessionOrderSortProxyModel(this);
  ordersProxyModel->setDynamicSortFilter(true);
  ordersProxyModel->setSourceModel(&ordersModel);
  orderView->setModel(ordersProxyModel);
  orderView->setSortingEnabled(true);
  orderView->setRootIsDecorated(false);
  orderView->setAlternatingRowColors(true);

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
  orderView->sortByColumn(1);
  settings.beginGroup("BotOrders");
  headerView->restoreState(settings.value("HeaderState").toByteArray());
  settings.endGroup();
  headerView->setStretchLastSection(false);
  headerView->setResizeMode(0, QHeaderView::Stretch);
}

void UserSessionOrdersWidget::saveState(QSettings& settings)
{
  settings.beginGroup("BotOrders");
  settings.setValue("HeaderState", orderView->header()->saveState());
  settings.endGroup();
}
