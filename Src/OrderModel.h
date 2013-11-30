
#pragma once

class OrderModel : public QAbstractItemModel
{
public:
  OrderModel(const Market& market);
  ~OrderModel();

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
    QString date;
    double amount;
    double price;
    double newAmount;
    double newPrice;
    enum class State
    {
      draft,
      open,
      canceled,
    } state;

    Order() : type(Type::unknown), amount(0.), price(0.), newAmount(0.), newPrice(0.), state(State::open) {}
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
      last = price,
  };

  void setData(const QList<Order>& order);

  int addOrder(Order::Type type, double price);

  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;

  const Order* getOrder(const QModelIndex& index) const;

private:
  const Market& market;
  QList<Order*> orders;
  QHash<QString, Order*> ordersById;
  QString openStr;
  QString draftStr;
  QString canceledStr;
  QString buyStr;
  QString sellStr;
  QString format;
  QFont italicFont;
  unsigned int nextDraftId;

  virtual QModelIndex parent(const QModelIndex& child) const;
  virtual int rowCount(const QModelIndex& parent) const;
  virtual int columnCount(const QModelIndex& parent) const;
  virtual Qt::ItemFlags flags(const QModelIndex &index) const;
  virtual QVariant data(const QModelIndex& index, int role) const;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  virtual bool setData(const QModelIndex & index, const QVariant & value, int role);
};
