
#pragma once

class UserBrokersWidget : public QWidget, public Entity::Listener
{
  Q_OBJECT

public:
  UserBrokersWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager, DataService& dataService);
  ~UserBrokersWidget();

  void saveState(QSettings& settings);

private:
  QTabFramework& tabFramework;
  Entity::Manager& entityManager;
  DataService& dataService;

  UserBrokersModel userBrokersModel;

  QTreeView* marketView;
  QSortFilterProxyModel* proxyModel;

  QSet<EUserBroker*> selection;
  quint64 selectedBrokerId;

  QAction* addAction;
  QAction* editAction;
  QAction* removeAction;

private slots:
  void addMarket();
  void editMarket();
  void removeMarket();
  void marketSelectionChanged();
  void marketDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);
  void marketDataRemoved(const QModelIndex& parent, int start, int end);
  void marketDataReset();
  void marketDataAdded(const QModelIndex& parent, int start, int end);

private:
  void updateTitle(EConnection& eDataService);
  void updateToolBarButtons();
  void updateSelection();

private: // Entity::Listener
  virtual void updatedEntitiy(Entity& oldEntity, Entity& newEntity);
};
