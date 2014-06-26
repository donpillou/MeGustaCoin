
#pragma once

class SessionItemModel : public QAbstractItemModel, public Entity::Listener
{
public:
  enum class Column
  {
      first,
      initialType = first,
      currentType,
      date,
      value,
      amount,
      price,
      profitablePrice,
      flipPrice,
      last = flipPrice,
  };

public:
  SessionItemModel(Entity::Manager& entityManager);
  ~SessionItemModel();

private:
  Entity::Manager& entityManager;
  EBotMarketAdapter* eBotMarketAdapter;
  QList<EBotSessionItem*> items;
  QVariant buyStr;
  QVariant sellStr;
  QVariant sellIcon;
  QVariant buyIcon;
  QString dateFormat;

private: // QAbstractItemModel
  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
  virtual QModelIndex parent(const QModelIndex& child) const;
  virtual int rowCount(const QModelIndex& parent) const;
  virtual int columnCount(const QModelIndex& parent) const;
  virtual QVariant data(const QModelIndex& index, int role) const;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private: // Entity::Listener
  virtual void addedEntity(Entity& entity);
  virtual void updatedEntitiy(Entity& oldEntity, Entity& newEntity);
  virtual void removedEntity(Entity& entity);
  virtual void removedAll(quint32 type);
};

class SessionItemSortProxyModel : public QSortFilterProxyModel
{
public:
  SessionItemSortProxyModel(QObject* parent) : QSortFilterProxyModel(parent) {}

private: // QSortFilterProxyModel
  virtual bool lessThan(const QModelIndex& left, const QModelIndex& right) const
  {
    const EBotSessionItem* leftItem = (const EBotSessionItem*)left.internalPointer();
    const EBotSessionItem* rightItem = (const EBotSessionItem*)right.internalPointer();
    switch((SessionItemModel::Column)left.column())
    {
    case SessionItemModel::Column::date:
      return leftItem->getDate().msecsTo(rightItem->getDate()) > 0;
    case SessionItemModel::Column::value:
      return leftItem->getAmount() * rightItem->getPrice() < rightItem->getAmount() * rightItem->getPrice();
    case SessionItemModel::Column::amount:
      return leftItem->getAmount() < rightItem->getAmount();
    case SessionItemModel::Column::price:
      return leftItem->getPrice() < rightItem->getPrice();
    case SessionItemModel::Column::flipPrice:
      return leftItem->getFlipPrice() < rightItem->getFlipPrice();
    default:
      break;
    }
    return QSortFilterProxyModel::lessThan(left, right);
  }
};
