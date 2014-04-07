
#pragma once

class OrderModel2 : public QAbstractItemModel, public Entity::Listener
{
public:
  OrderModel2(Entity::Manager& entityManager);
  ~OrderModel2();

  //enum class State
  //{
  //  empty,
  //  loading,
  //  loaded,
  //  error,
  //};

  //class Order
  //{
  //public:
  //  QString id;
  //  enum class Type
  //  {
  //    unknown,
  //    buy,
  //    sell,
  //  } type;
  //  QDateTime date;
  //  double amount;
  //  double price;
  //  double total;
  //  double newAmount;
  //  double newPrice;
  //  enum class State
  //  {
  //    draft,
  //    submitting,
  //    open,
  //    canceling,
  //    canceled,
  //    closed,
  //  } state;
  //
  //  Order() : type(Type::unknown), amount(0.), price(0.), newAmount(0.), newPrice(0.), state(State::open) {}
  //
  //  Order& operator=(const Market::Order& order)
  //  {
  //    id = order.id;
  //    date = QDateTime::fromTime_t(order.date).toLocalTime();
  //    amount = fabs(order.amount);
  //    price = order.price;
  //    total = order.total;
  //    newAmount = newPrice = 0.;
  //    state = Order::State::open;
  //    type = order.amount > 0. ? Type::buy : Type::sell;
  //    return *this;
  //  }
  //};

  enum class Column
  {
      first,
      type = first,
      state,
      date,
      value,
      amount,
      price,
      total,
      last = total,
  };

  //void setState(State state);
  //QString getStateName() const;

//signals:
//  void editedOrder(const QModelIndex& index);
//  void changedState();
//  void editedDraft(const QModelIndex& index);

private:
  Entity::Manager& entityManager;
  EMarket* eMarket;
  QList<EOrder*> orders;
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

class OrderSortProxyModel2 : public QSortFilterProxyModel
{
public:
  OrderSortProxyModel2(QObject* parent) : QSortFilterProxyModel(parent) {}

private:
  virtual bool lessThan(const QModelIndex& left, const QModelIndex& right) const
  {
    const EOrder* leftOrder = (const EOrder*)left.internalPointer();
    const EOrder* rightOrder = (const EOrder*)right.internalPointer();
    switch((OrderModel2::Column)left.column())
    {
    case OrderModel2::Column::date:
      return leftOrder->getDate().msecsTo(rightOrder->getDate()) > 0;
    case OrderModel2::Column::value:
      return leftOrder->getAmount() * leftOrder->getPrice() < rightOrder->getAmount() * rightOrder->getPrice();
    case OrderModel2::Column::amount:
      return leftOrder->getAmount() < rightOrder->getAmount();
    case OrderModel2::Column::price:
      return leftOrder->getPrice() < rightOrder->getPrice();
    case OrderModel2::Column::total:
      return leftOrder->getTotal() < rightOrder->getTotal();
    default:
      break;
    }
    return QSortFilterProxyModel::lessThan(left, right);
  }
};
