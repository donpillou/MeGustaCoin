
#pragma once

class UserSessionAssetsWidget : public QWidget, public Entity::Listener
{
  Q_OBJECT

public:
  UserSessionAssetsWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager, DataService& dataService);
  ~UserSessionAssetsWidget();

  void saveState(QSettings& settings);

private slots:
  void newBuyItem();
  void newSellItem();
  void submitItem();
  void cancelItem();
  void importAssets();
  void exportAssets();
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
  QAction* importAction;
  QAction* exportAction;

private:
  void addSessionItemDraft(EUserSessionAsset::Type type);
  void updateToolBarButtons();

private: // Entity::Listener
  virtual void updatedEntitiy(Entity& oldEntity, Entity& newEntity);
};
