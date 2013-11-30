
#pragma once

class OrderWidget : public QWidget
{
  Q_OBJECT

public:
  OrderWidget(QWidget* parent, QSettings& settings);

public slots:
  void setMarket(Market* market);
  void newBuyOrder();
  void newSellOrder();
  void submitOrder();
  void cancelOrder();

private:
  QSettings& settings;
  Market* market;
  QTreeView* orderView;
  QSortFilterProxyModel* orderProxyModel;

  void addOrder(OrderModel::Order::Type type);
};

