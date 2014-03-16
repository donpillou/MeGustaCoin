
#pragma once

class DataModel;

class OrderModel : public QAbstractItemModel
{
  Q_OBJECT

public:
  OrderModel(DataModel& dataModel);
  ~OrderModel();

  enum class State
  {
    empty,
    loading,
    loaded,
    error,
  };

  class Order
  {
  public:
    QString id;
    enum class Type
    {
      unknown,
      buy,
      sell,
    } type;
    QDateTime date;
    double amount;
    double price;
    double total;
    double newAmount;
    double newPrice;
    enum class State
    {
      draft,
      submitting,
      open,
      canceling,
      canceled,
      closed,
    } state;

    Order() : type(Type::unknown), amount(0.), price(0.), newAmount(0.), newPrice(0.), state(State::open) {}

    Order& operator=(const Market::Order& order)
    {
      id = order.id;
      date = QDateTime::fromTime_t(order.date).toLocalTime();
      amount = fabs(order.amount);
      price = order.price;
      total = order.total;
      newAmount = newPrice = 0.;
      state = Order::State::open;
      type = order.amount > 0. ? Type::buy : Type::sell;
      return *this;
    }
  };

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

  void setState(State state);
  QString getStateName() const;

  void reset();

  void setData(const QList<Market::Order>& orders);
  void addOrder(const Market::Order& order);
  void removeOrder(const QString& id);
  void updateOrder(const QString& id, const Market::Order& order);
  void setOrderState(const QString& id, Order::State state);
  void setOrderNewAmount(const QString& id, double newAmount);

  QString addOrderDraft(Order::Type type, double price);

  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;

  QModelIndex getOrderIndex(const QString& id) const;
  const Order* getOrder(const QString& id) const;
  const Order* getOrder(const QModelIndex& index) const;
  void removeOrder(const QModelIndex& index);

signals:
  void editedOrder(const QModelIndex& index);
  void changedState();
  void editedDraft(const QModelIndex& index);

private:
  DataModel& dataModel;
  QList<Order*> orders;
  QVariant draftStr;
  QVariant submittingStr;
  QVariant openStr;
  QVariant cancelingStr;
  QVariant canceledStr;
  QVariant closedStr;
  QVariant buyStr;
  QVariant sellStr;
  QVariant format;
  QFont italicFont;
  QVariant sellIcon;
  QVariant buyIcon;
  QString dateFormat;
  unsigned int nextDraftId;
  State state;

  virtual QModelIndex parent(const QModelIndex& child) const;
  virtual int rowCount(const QModelIndex& parent) const;
  virtual int columnCount(const QModelIndex& parent) const;
  virtual Qt::ItemFlags flags(const QModelIndex &index) const;
  virtual QVariant data(const QModelIndex& index, int role) const;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  virtual bool setData(const QModelIndex & index, const QVariant & value, int role);

private slots:
  void updateHeader();
};

class OrderSortProxyModel : public QSortFilterProxyModel
{
public:
  OrderSortProxyModel(QObject* parent, OrderModel& orderModel) : QSortFilterProxyModel(parent), orderModel(orderModel) {}

private:
  OrderModel& orderModel;

  virtual bool lessThan(const QModelIndex& left, const QModelIndex& right) const
  {
    const OrderModel::Order* leftOrder = orderModel.getOrder(left);
    const OrderModel::Order* rightOrder = orderModel.getOrder(right);
    switch((OrderModel::Column)left.column())
    {
    case OrderModel::Column::date:
      return leftOrder->date.msecsTo(rightOrder->date) > 0;
    case OrderModel::Column::value:
      return leftOrder->amount * leftOrder->price < rightOrder->amount * rightOrder->price;
    case OrderModel::Column::amount:
      return leftOrder->amount < rightOrder->amount;
    case OrderModel::Column::price:
      return leftOrder->price < rightOrder->price;
    case OrderModel::Column::total:
      return leftOrder->total < rightOrder->total;
    default:
      break;
    }
    return QSortFilterProxyModel::lessThan(left, right);
  }
};
