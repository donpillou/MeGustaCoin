
#pragma once

class UserBrokerTransactionsModel : public QAbstractItemModel, public Entity::Listener
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
  UserBrokerTransactionsModel(Entity::Manager& entityManager);
  ~UserBrokerTransactionsModel();

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

class UserBrokerTransactionsSortProxyModel : public QSortFilterProxyModel
{
public:
  UserBrokerTransactionsSortProxyModel(QObject* parent) : QSortFilterProxyModel(parent) {}

private: // QSortFilterProxyModel
  virtual bool lessThan(const QModelIndex& left, const QModelIndex& right) const
  {
    const EUserBrokerTransaction* leftTransaction = (const EUserBrokerTransaction*)left.internalPointer();
    const EUserBrokerTransaction* rightTransaction = (const EUserBrokerTransaction*)right.internalPointer();
    switch((UserBrokerTransactionsModel::Column)left.column())
    {
    case UserBrokerTransactionsModel::Column::date:
      return leftTransaction->getDate().msecsTo(rightTransaction->getDate()) > 0;
    case UserBrokerTransactionsModel::Column::value:
      return leftTransaction->getAmount() * leftTransaction->getPrice() < rightTransaction->getAmount() * rightTransaction->getPrice();
    case UserBrokerTransactionsModel::Column::amount:
      return leftTransaction->getAmount() < rightTransaction->getAmount();
    case UserBrokerTransactionsModel::Column::price:
      return leftTransaction->getPrice() < rightTransaction->getPrice();
    case UserBrokerTransactionsModel::Column::fee:
      return leftTransaction->getFee() < rightTransaction->getFee();
    case UserBrokerTransactionsModel::Column::total:
      return leftTransaction->getTotal() < rightTransaction->getTotal();
    default:
      break;
    }
    return QSortFilterProxyModel::lessThan(left, right);
  }
};
