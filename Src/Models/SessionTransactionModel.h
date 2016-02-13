
#pragma once

class SessionTransactionModel : public QAbstractItemModel, public Entity::Listener
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
  SessionTransactionModel(Entity::Manager& entityManager);
  ~SessionTransactionModel();

private:
  Entity::Manager& entityManager;
  EBrokerType* eBrokerType;
  QList<EUserSessionTransaction*> transactions;
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

class SessionTransactionSortProxyModel : public QSortFilterProxyModel
{
public:
  SessionTransactionSortProxyModel(QObject* parent) : QSortFilterProxyModel(parent) {}

private: // QSortFilterProxyModel
  virtual bool lessThan(const QModelIndex& left, const QModelIndex& right) const
  {
    const EUserSessionTransaction* leftTransaction = (const EUserSessionTransaction*)left.internalPointer();
    const EUserSessionTransaction* rightTransaction = (const EUserSessionTransaction*)right.internalPointer();
    switch((SessionTransactionModel::Column)left.column())
    {
    case SessionTransactionModel::Column::date:
      return leftTransaction->getDate().msecsTo(rightTransaction->getDate()) > 0;
    case SessionTransactionModel::Column::value:
      return leftTransaction->getAmount() * leftTransaction->getPrice() < rightTransaction->getAmount() * rightTransaction->getPrice();
    case SessionTransactionModel::Column::amount:
      return leftTransaction->getAmount() < rightTransaction->getAmount();
    case SessionTransactionModel::Column::price:
      return leftTransaction->getPrice() < rightTransaction->getPrice();
    case SessionTransactionModel::Column::fee:
      return leftTransaction->getFee() < rightTransaction->getFee();
    case SessionTransactionModel::Column::total:
      return leftTransaction->getTotal() < rightTransaction->getTotal();
    default:
      break;
    }
    return QSortFilterProxyModel::lessThan(left, right);
  }
};
