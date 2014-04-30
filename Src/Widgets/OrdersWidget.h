
#pragma once

class OrdersWidget : public QWidget, public Entity::Listener
{
  Q_OBJECT

public:
  OrdersWidget(QWidget* parent, QSettings& settings, Entity::Manager& entityManager, BotService& botService);
  ~OrdersWidget();

  void saveState(QSettings& settings);

public slots:
  void refresh();

private slots:
  void newBuyOrder();
  void newSellOrder();
  void submitOrder();
  void cancelOrder();
  void updateOrder(const QModelIndex& index);
  void updateDraft(const QModelIndex& index);
  void updateToolBarButtons();

private:
  Entity::Manager& entityManager;
  BotService& botService;

  MarketOrderModel orderModel;

  QTreeView* orderView;
  QSortFilterProxyModel* proxyModel;

  QAction* refreshAction;
  QAction* buyAction;
  QAction* sellAction;
  QAction* submitAction;
  QAction* cancelAction;

  void addOrder(OrderModel::Order::Type type);
  QList<QModelIndex> getSelectedRows();

private:
    void updateTitle(EBotService& eBotService);

private: // Entity::Listener
  virtual void updatedEntitiy(Entity& oldEntity, Entity& newEntity);

};
