
#pragma once

class SessionOrderModel : public QAbstractItemModel, public Entity::Listener
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
  SessionOrderModel(Entity::Manager& entityManager);
  ~SessionOrderModel();

private:
  Entity::Manager& entityManager;
  EBotMarketAdapter* eBotMarketAdapter;
  QList<EBotSessionOrder*> orders;
  QVariant draftStr;
  QVariant submittingStr;
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

class SessionOrderSortProxyModel : public QSortFilterProxyModel
{
public:
  SessionOrderSortProxyModel(QObject* parent) : QSortFilterProxyModel(parent) {}

private:
  virtual bool lessThan(const QModelIndex& left, const QModelIndex& right) const
  {
    const EBotSessionOrder* leftOrder = (const EBotSessionOrder*)left.internalPointer();
    const EBotSessionOrder* rightOrder = (const EBotSessionOrder*)right.internalPointer();
    switch((SessionOrderModel::Column)left.column())
    {
    case SessionOrderModel::Column::date:
      return leftOrder->getDate().msecsTo(rightOrder->getDate()) > 0;
    case SessionOrderModel::Column::value:
      return leftOrder->getAmount() * leftOrder->getPrice() < rightOrder->getAmount() * rightOrder->getPrice();
    case SessionOrderModel::Column::amount:
      return leftOrder->getAmount() < rightOrder->getAmount();
    case SessionOrderModel::Column::price:
      return leftOrder->getPrice() < rightOrder->getPrice();
    case SessionOrderModel::Column::total:
      return leftOrder->getTotal() < rightOrder->getTotal();
    default:
      break;
    }
    return QSortFilterProxyModel::lessThan(left, right);
  }
};
