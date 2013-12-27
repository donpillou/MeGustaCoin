
#pragma once

class OrdersWidget : public QWidget
{
  Q_OBJECT

public:
  OrdersWidget(QWidget* parent, QSettings& settings, DataModel& dataModel);

  void saveState(QSettings& settings);

public slots:
  void refresh();

private slots:
  void setMarket(Market* market);
  void newBuyOrder();
  void newSellOrder();
  void submitOrder();
  void cancelOrder();
  void updateOrder(const QModelIndex& index);
  void updateToolBarButtons();

private:
  DataModel& dataModel;
  OrderModel& orderModel;
  Market* market;
  QTreeView* orderView;
  QSortFilterProxyModel* proxyModel;

  QAction* refreshAction;
  QAction* buyAction;
  QAction* sellAction;
  QAction* submitAction;
  QAction* cancelAction;

  void addOrder(OrderModel::Order::Type type);
  QList<QModelIndex> getSelectedRows();
};

