
#pragma once

class BotsWidget : public QWidget, public Entity::Listener
{
  Q_OBJECT

public:
  BotsWidget(QWidget* parent, QSettings& settings, Entity::Manager& entityManager, BotService& botService);
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
  void checkAutoScroll(const QModelIndex&, int, int);
  void autoScroll(int, int);

private:
  Entity::Manager& entityManager;
  BotService& botService;

  BotSessionModel botSessionModel;
  SessionOrderModel orderModel;
  SessionTransactionModel transactionModel;
  SessionLogModel logModel;

  QSplitter* splitter;
  QTreeView* sessionView;
  QTreeView* orderView;
  QTreeView* transactionView;
  QTreeView* logView;
  QSortFilterProxyModel* sessionProxyModel;
  bool autoScrollEnabled;

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
