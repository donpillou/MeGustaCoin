
#pragma once

class UserSessionAssetsModel : public QAbstractItemModel, public Entity::Listener
{
Q_OBJECT

public:
  enum class Column
  {
      first,
      type = first,
      state,
      date,
      investBase,
      investComm,
      balanceBase,
      balanceComm,
      price,
      profitablePrice,
      flipPrice,
      last = flipPrice,
  };

public:
  UserSessionAssetsModel(Entity::Manager& entityManager);
  ~UserSessionAssetsModel();

  QModelIndex getDraftAmountIndex(EUserSessionAssetDraft& draft);

signals:
  void editedItemFlipPrice(const QModelIndex& index, double price);

private:
  Entity::Manager& entityManager;
  EBrokerType* eBrokerType;
  QList<EUserSessionAsset*> items;
  QVariant draftStr;
  QVariant buyStr;
  QVariant sellStr;
  QVariant buyingStr;
  QVariant sellingStr;
  QVariant sellIcon;
  QVariant buyIcon;
  QString dateFormat;

private: // QAbstractItemModel
  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
  virtual QModelIndex parent(const QModelIndex& child) const;
  virtual int rowCount(const QModelIndex& parent) const;
  virtual int columnCount(const QModelIndex& parent) const;
  virtual QVariant data(const QModelIndex& index, int role) const;
  virtual Qt::ItemFlags flags(const QModelIndex &index) const;
  virtual bool setData(const QModelIndex & index, const QVariant & value, int role);
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private: // Entity::Listener
  virtual void addedEntity(Entity& entity);
  virtual void updatedEntitiy(Entity& oldEntity, Entity& newEntity);
  virtual void addedEntity(Entity& entity, Entity& replacedEntity);
  virtual void removedEntity(Entity& entity);
  virtual void removedAll(quint32 type);
};

class UserSessionAssetsSortProxyModel : public QSortFilterProxyModel
{
public:
  UserSessionAssetsSortProxyModel(QObject* parent) : QSortFilterProxyModel(parent) {}

private: // QSortFilterProxyModel
  virtual bool lessThan(const QModelIndex& left, const QModelIndex& right) const
  {
    const EUserSessionAsset* leftItem = (const EUserSessionAsset*)left.internalPointer();
    const EUserSessionAsset* rightItem = (const EUserSessionAsset*)right.internalPointer();
    switch((UserSessionAssetsModel::Column)left.column())
    {
    case UserSessionAssetsModel::Column::date:
      return leftItem->getDate().msecsTo(rightItem->getDate()) > 0;
    case UserSessionAssetsModel::Column::balanceComm:
      return leftItem->getBalanceComm() < rightItem->getBalanceComm();
    case UserSessionAssetsModel::Column::balanceBase:
      return leftItem->getBalanceBase() < rightItem->getBalanceBase();
    case UserSessionAssetsModel::Column::price:
      return leftItem->getPrice() < rightItem->getPrice();
    case UserSessionAssetsModel::Column::flipPrice:
      return leftItem->getFlipPrice() < rightItem->getFlipPrice();
    default:
      break;
    }
    return QSortFilterProxyModel::lessThan(left, right);
  }
};
