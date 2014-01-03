
#pragma once

class OrdersWidget : public QWidget
{
  Q_OBJECT

public:
  OrdersWidget(QWidget* parent, QSettings& settings, DataModel& dataModel, MarketService& marketService);

  void saveState(QSettings& settings);

public slots:
  void refresh();

private slots:
  void newBuyOrder();
  void newSellOrder();
  void submitOrder();
  void cancelOrder();
  void updateOrder(const QModelIndex& index);
  void updateToolBarButtons();
  void updateTitle();

private:
  DataModel& dataModel;
  OrderModel& orderModel;
  MarketService& marketService;

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
