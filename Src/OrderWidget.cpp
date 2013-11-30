
#include "stdafx.h"

OrderWidget::OrderWidget(QWidget* parent) : QWidget(parent)
{
  orderView = new QTreeView(this);
  orderProxyModel = new QSortFilterProxyModel(this);
  orderProxyModel->setDynamicSortFilter(true);
  orderView->setModel(orderProxyModel);
  //orderView->setIndentation(0);
  orderView->setSortingEnabled(true);
  orderView->setRootIsDecorated(false);
  orderView->setAlternatingRowColors(true);

}
