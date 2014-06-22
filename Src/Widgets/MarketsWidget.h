
#pragma once

class MarketsWidget : public QWidget, public Entity::Listener
{
  Q_OBJECT

public:
  MarketsWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager, BotService& botService);
  ~MarketsWidget();

  void saveState(QSettings& settings);

private:
  QTabFramework& tabFramework;
  Entity::Manager& entityManager;
  BotService& botService;

  QTreeView* marketView;
  QSortFilterProxyModel* proxyModel;

  QAction* addAction;
  QAction* editAction;
  QAction* removeAction;

  BotMarketModel botMarketModel;

private slots:
  void addMarket();
  void editMarket();
  void removeMarket();
  void updateToolBarButtons();
  void marketSelectionChanged();

private:
  void updateTitle(EBotService& eBotService);

private: // Entity::Listener
  virtual void updatedEntitiy(Entity& oldEntity, Entity& newEntity);
};
