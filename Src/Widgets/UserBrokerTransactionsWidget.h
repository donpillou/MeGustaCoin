
#pragma once

class UserBrokerTransactionsWidget : public QWidget, public Entity::Listener
{
  Q_OBJECT

public:
  UserBrokerTransactionsWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager, DataService& dataService);
  ~UserBrokerTransactionsWidget();

  void saveState(QSettings& settings);

public slots:
  void refresh();

private slots:
  void updateToolBarButtons();

private:
  QTabFramework& tabFramework;
  Entity::Manager& entityManager;
  DataService& dataService;

  UserBrokerTransactionsModel transactionsModel;

  QTreeView* transactionView;
  QSortFilterProxyModel* proxyModel;

  QAction* refreshAction;

private:
  void updateTitle(EConnection& eDataService);

private: // Entity::Listener
  virtual void updatedEntitiy(Entity& oldEntity, Entity& newEntity);
};

