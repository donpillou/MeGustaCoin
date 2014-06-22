
#pragma once

class BotsWidget : public QWidget, public Entity::Listener
{
  Q_OBJECT

public:
  BotsWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager, BotService& botService);
  ~BotsWidget();

  void saveState(QSettings& settings);

private slots:
  void addBot();
  void optimize();
  void simulate();
  void activate();
  void cancelBot();
  void updateToolBarButtons();
  void sessionSelectionChanged();

private:
  QTabFramework& tabFramework;
  Entity::Manager& entityManager;
  BotService& botService;

  BotSessionModel botSessionModel;
  SessionOrderModel orderModel;
  SessionTransactionModel transactionModel;

  QSplitter* splitter;
  QTreeView* sessionView;
  QTreeView* transactionView;
  QSortFilterProxyModel* sessionProxyModel;

  QAction* addAction;
  QAction* optimizeAction;
  QAction* simulateAction;
  QAction* activateAction;
  QAction* cancelAction;

private:
  void updateTitle(EBotService& eBotService);

private: // Entity::Listener
  virtual void updatedEntitiy(Entity& oldEntity, Entity& newEntity);
};
