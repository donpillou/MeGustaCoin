
#pragma once

class MarketTransactionModel : public QAbstractItemModel, public Entity::Listener
{
public:
  enum class Column
  {
      first,
      type = first,
      date,
      value,
      amount,
      price,
      fee,
      total,
      last = total,
  };

public:
  MarketTransactionModel(Entity::Manager& entityManager);
  ~MarketTransactionModel();

private:
  Entity::Manager& entityManager;
  EBrokerType* eBrokerType;
  QList<EUserBrokerTransaction*> transactions;
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

class MarketTransactionSortProxyModel : public QSortFilterProxyModel
{
public:
  MarketTransactionSortProxyModel(QObject* parent) : QSortFilterProxyModel(parent) {}

private: // QSortFilterProxyModel
  virtual bool lessThan(const QModelIndex& left, const QModelIndex& right) const
  {
    const EUserBrokerTransaction* leftTransaction = (const EUserBrokerTransaction*)left.internalPointer();
    const EUserBrokerTransaction* rightTransaction = (const EUserBrokerTransaction*)right.internalPointer();
    switch((MarketTransactionModel::Column)left.column())
    {
    case MarketTransactionModel::Column::date:
      return leftTransaction->getDate().msecsTo(rightTransaction->getDate()) > 0;
    case MarketTransactionModel::Column::value:
      return leftTransaction->getAmount() * leftTransaction->getPrice() < rightTransaction->getAmount() * rightTransaction->getPrice();
    case MarketTransactionModel::Column::amount:
      return leftTransaction->getAmount() < rightTransaction->getAmount();
    case MarketTransactionModel::Column::price:
      return leftTransaction->getPrice() < rightTransaction->getPrice();
    case MarketTransactionModel::Column::fee:
      return leftTransaction->getFee() < rightTransaction->getFee();
    case MarketTransactionModel::Column::total:
      return leftTransaction->getTotal() < rightTransaction->getTotal();
    default:
      break;
    }
    return QSortFilterProxyModel::lessThan(left, right);
  }
};
