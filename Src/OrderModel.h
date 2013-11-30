
#pragma once

class OrderModel : public QAbstractItemModel
{
public:
  OrderModel();

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
    enum class State
    {
      unknown,
      open,
      deleted,
    } state;

    Order() : type(Type::unknown), amount(0.), price(0.), state(State::open) {}
  };

  void setData(const QList<Order>& order);

private:
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

  QList<Order*> orders;
  QHash<QString, Order*> ordersById;
  QString openStr;
  QString deletedStr;
  QString buyStr;
  QString sellStr;

  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
  virtual QModelIndex parent(const QModelIndex& child) const;
  virtual int rowCount(const QModelIndex& parent) const;
  virtual int columnCount(const QModelIndex& parent) const;
  virtual Qt::ItemFlags flags(const QModelIndex &index) const;
  virtual QVariant data(const QModelIndex& index, int role) const;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
};
