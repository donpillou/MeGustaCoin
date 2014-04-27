
#pragma once

class TransactionModel2 : public QAbstractItemModel, public Entity::Listener
{
public:
  TransactionModel2(Entity::Manager& entityManager);
  ~TransactionModel2();

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

private:
  Entity::Manager& entityManager;
  EBotMarketAdapter* eBotMarketAdapter;
  QList<EBotSessionTransaction*> transactions;
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

class TransactionSortProxyModel2 : public QSortFilterProxyModel
{
public:
  TransactionSortProxyModel2(QObject* parent) : QSortFilterProxyModel(parent) {}

private: // QSortFilterProxyModel
  virtual bool lessThan(const QModelIndex& left, const QModelIndex& right) const
  {
    const EBotSessionTransaction* leftTransaction = (const EBotSessionTransaction*)left.internalPointer();
    const EBotSessionTransaction* rightTransaction = (const EBotSessionTransaction*)right.internalPointer();
    switch((TransactionModel2::Column)left.column())
    {
    case TransactionModel2::Column::date:
      return leftTransaction->getDate().msecsTo(rightTransaction->getDate()) > 0;
    case TransactionModel2::Column::value:
      return leftTransaction->getAmount() * leftTransaction->getPrice() < rightTransaction->getAmount() * rightTransaction->getPrice();
    case TransactionModel2::Column::amount:
      return leftTransaction->getAmount() < rightTransaction->getAmount();
    case TransactionModel2::Column::price:
      return leftTransaction->getPrice() < rightTransaction->getPrice();
    case TransactionModel2::Column::fee:
      return leftTransaction->getFee() < rightTransaction->getFee();
    case TransactionModel2::Column::total:
      return leftTransaction->getTotal() < rightTransaction->getTotal();
    default:
      break;
    }
    return QSortFilterProxyModel::lessThan(left, right);
  }
};
