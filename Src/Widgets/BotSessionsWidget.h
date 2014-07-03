
#pragma once

class BotSessionsWidget : public QWidget, public Entity::Listener
{
  Q_OBJECT

public:
  BotSessionsWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager, BotService& botService);
  ~BotSessionsWidget();

  void saveState(QSettings& settings);

private slots:
  void addBot();
  void optimize();
  void simulate();
  void activate();
  void cancelBot();
  void sessionSelectionChanged();
  void sessionDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);

private:
  QTabFramework& tabFramework;
  Entity::Manager& entityManager;
  BotService& botService;

  BotSessionModel botSessionModel;
  SessionOrderModel orderModel;
  SessionTransactionModel transactionModel;

  QTreeView* sessionView;
  QSortFilterProxyModel* proxyModel;

  QSet<EBotSession*> selection;

  QAction* addAction;
  QAction* optimizeAction;
  QAction* simulateAction;
  QAction* activateAction;
  QAction* cancelAction;

private:
  void updateTitle(EBotService& eBotService);
  void updateToolBarButtons();
  void updateSelection();

private: // Entity::Listener
  virtual void updatedEntitiy(Entity& oldEntity, Entity& newEntity);
};
