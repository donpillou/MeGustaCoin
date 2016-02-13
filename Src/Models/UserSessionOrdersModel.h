
#pragma once

class UserSessionOrdersModel : public QAbstractItemModel, public Entity::Listener
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
      total,
      last = total,
  };

public:
  UserSessionOrdersModel(Entity::Manager& entityManager);
  ~UserSessionOrdersModel();

private:
  Entity::Manager& entityManager;
  EBrokerType* eBrokerType;
  QList<EUserSessionOrder*> orders;
  QVariant draftStr;
  QVariant submittingStr;
  QVariant updatingStr;
  QVariant openStr;
  QVariant cancelingStr;
  QVariant canceledStr;
  QVariant closedStr;
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

class UserSessionOrderSortProxyModel : public QSortFilterProxyModel
{
public:
  UserSessionOrderSortProxyModel(QObject* parent) : QSortFilterProxyModel(parent) {}

private:
  virtual bool lessThan(const QModelIndex& left, const QModelIndex& right) const
  {
    const EUserSessionOrder* leftOrder = (const EUserSessionOrder*)left.internalPointer();
    const EUserSessionOrder* rightOrder = (const EUserSessionOrder*)right.internalPointer();
    switch((UserSessionOrdersModel::Column)left.column())
    {
    case UserSessionOrdersModel::Column::date:
      return leftOrder->getDate().msecsTo(rightOrder->getDate()) > 0;
    case UserSessionOrdersModel::Column::value:
      return leftOrder->getAmount() * leftOrder->getPrice() < rightOrder->getAmount() * rightOrder->getPrice();
    case UserSessionOrdersModel::Column::amount:
      return leftOrder->getAmount() < rightOrder->getAmount();
    case UserSessionOrdersModel::Column::price:
      return leftOrder->getPrice() < rightOrder->getPrice();
    case UserSessionOrdersModel::Column::total:
      return leftOrder->getTotal() < rightOrder->getTotal();
    default:
      break;
    }
    return QSortFilterProxyModel::lessThan(left, right);
  }
};
