
#pragma once

class TransactionsWidget : public QWidget, public Entity::Listener
{
  Q_OBJECT

public:
  TransactionsWidget(QWidget* parent, QSettings& settings, Entity::Manager& entityManager, BotService& botService);
  ~TransactionsWidget();

  void saveState(QSettings& settings);

public slots:
  void refresh();

private slots:
  void updateToolBarButtons();

private:
  Entity::Manager& entityManager;
  BotService& botService;

  MarketTransactionModel transactionModel;

  QTreeView* transactionView;
  QSortFilterProxyModel* proxyModel;

  QAction* refreshAction;

private:
  void updateTitle(EBotService& eBotService);

private: // Entity::Listener
  virtual void updatedEntitiy(Entity& oldEntity, Entity& newEntity);
};

