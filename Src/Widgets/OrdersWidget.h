
#pragma once

class OrdersWidget : public QWidget, public Entity::Listener
{
  Q_OBJECT

public:
  OrdersWidget(QWidget* parent, QSettings& settings, Entity::Manager& entityManager, BotService& botServic, DataModel& dataModele);
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
  DataModel& dataModel; // todo: get rid of this

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
