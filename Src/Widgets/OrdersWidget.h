
#pragma once

class OrdersWidget : public QWidget, public Entity::Listener
{
  Q_OBJECT

public:
  OrdersWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager, BotService& botService, DataService& dataService);
  ~OrdersWidget();

  void saveState(QSettings& settings);

public slots:
  void refresh();

private slots:
  void newBuyOrder();
  void newSellOrder();
  void submitOrder();
  void cancelOrder();
  void editedOrderPrice(const QModelIndex& index, double price);
  void editedOrderAmount(const QModelIndex& index, double amount);
  void updateToolBarButtons();

private:
  QTabFramework& tabFramework;
  Entity::Manager& entityManager;
  BotService& botService;
  DataService& dataService;

  MarketOrderModel orderModel;

  QTreeView* orderView;
  QSortFilterProxyModel* proxyModel;

  QAction* refreshAction;
  QAction* buyAction;
  QAction* sellAction;
  QAction* submitAction;
  QAction* cancelAction;

  void addOrderDraft(EBotMarketOrder::Type type);

private:
    void updateTitle(EBotService& eBotService);

private: // Entity::Listener
  virtual void updatedEntitiy(Entity& oldEntity, Entity& newEntity);

};
