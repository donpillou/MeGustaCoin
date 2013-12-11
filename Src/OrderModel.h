
#pragma once

class OrderModel : public QAbstractItemModel
{
  Q_OBJECT

public:
  OrderModel();
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
      submitting,
      open,
      canceling,
      canceled,
      closed,
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

  void setCurrencies(const QString& market, const QString& coin);

  void reset();

  void setData(const QList<Order>& order);
  void updateOrder(const QString& id, const Order& order);
  void setOrderState(const QString& id, Order::State state);
  void setOrderNewAmount(const QString& id, double newAmount);

  int addOrder(Order::Type type, double price);

  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;

  const Order* getOrder(const QModelIndex& index) const;
  void removeOrder(const QModelIndex& index);

signals:
  void orderEdited(const QModelIndex& index);

private:
  QList<Order*> orders;
  QByteArray marketCurrency;
  QByteArray coinCurrency;
  QString draftStr;
  QString submittingStr;
  QString openStr;
  QString cancelingStr;
  QString canceledStr;
  QString closedStr;
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
