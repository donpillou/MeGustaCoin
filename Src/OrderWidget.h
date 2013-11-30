
#pragma once

class OrderWidget : public QWidget
{
public:
  OrderWidget(QWidget* parent);

private:
  QTreeView* orderView;
  QSortFilterProxyModel* orderProxyModel;
};

