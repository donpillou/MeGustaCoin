
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
  void simulate(bool enabled);
  void activate(bool enabled);

private:
  Entity::Manager& entityManager;
  BotService& botService;

  BotSessionModel botSessionModel;
  OrderModel2 orderModel;
  TransactionModel2 transactionModel;

  QSplitter* splitter;
  QTreeView* sessionView;
  QTreeView* orderView;
  QTreeView* transactionView;

  QAction* addAction;
  QAction* optimizeAction;
  QAction* simulateAction;
  QAction* activateAction;

private:
  void updateTitle(EBotService& eBotService);
  void updateToolBarButtons();

private: // Entity::Listener
  virtual void updatedEntitiy(Entity& entity);
};
