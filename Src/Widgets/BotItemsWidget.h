
#pragma once

class BotItemsWidget : public QWidget, public Entity::Listener
{
  Q_OBJECT

public:
  BotItemsWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager, DataService& dataService);
  ~BotItemsWidget();

  void saveState(QSettings& settings);

private slots:
  void newBuyItem();
  void newSellItem();
  void submitItem();
  void cancelItem();
  void itemSelectionChanged();
  void itemDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);
  void editedItemFlipPrice(const QModelIndex& index, double price);

private:
  Entity::Manager& entityManager;
  DataService& dataService;

  UserSessionAssetsModel assetsModel;

  QTreeView* itemView;
  QSortFilterProxyModel* proxyModel;

  QSet<EUserSessionAsset*> selection;

  QAction* buyAction;
  QAction* sellAction;
  QAction* submitAction;
  QAction* cancelAction;

private:
  void addSessionItemDraft(EUserSessionAsset::Type type);
  void updateToolBarButtons();

private: // Entity::Listener
  virtual void updatedEntitiy(Entity& oldEntity, Entity& newEntity);
};
