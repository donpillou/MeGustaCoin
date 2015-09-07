
#pragma once

class TransactionsWidget : public QWidget, public Entity::Listener
{
  Q_OBJECT

public:
  TransactionsWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager, DataService& dataService);
  ~TransactionsWidget();

  void saveState(QSettings& settings);

public slots:
  void refresh();

private slots:
  void updateToolBarButtons();

private:
  QTabFramework& tabFramework;
  Entity::Manager& entityManager;
  DataService& dataService;

  MarketTransactionModel transactionModel;

  QTreeView* transactionView;
  QSortFilterProxyModel* proxyModel;

  QAction* refreshAction;

private:
  void updateTitle(EDataService& eDataService);

private: // Entity::Listener
  virtual void updatedEntitiy(Entity& oldEntity, Entity& newEntity);
};

