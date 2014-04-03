
#pragma once

class TransactionModel2 : public QAbstractItemModel, public Entity::Listener
{
public:
  TransactionModel2(Entity::Manager& entityManager);
  ~TransactionModel2();

  /*
  enum class State
  {
    empty,
    loading,
    loaded,
    error,
  };
  */

  class Transaction
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
    double fee;
    double total;

    Transaction() : type(Type::unknown), amount(0.), price(0.), fee(0.), total(0.) {}

    Transaction& operator=(const Market::Transaction& transaction)
    {
      id = transaction.id;
      date = QDateTime::fromTime_t(transaction.date).toLocalTime();
      amount = fabs(transaction.amount);
      price = transaction.price;
      fee = transaction.fee;
      total = transaction.total;
      type = transaction.amount > 0. ? Transaction::Type::buy : Transaction::Type::sell;
      return *this;
    }
  };

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

  //void setState(State state);
  //QString getStateName() const;

  void reset();

  void setData(const QList<Market::Transaction>& transactions);
  void addTransaction(const Market::Transaction& transaction);
  void removeTransaction(const QString& id);

  const Transaction* getTransaction(const QModelIndex& index) const;

//signals:
//  void changedState();

private:
  Entity::Manager& entityManager;
  EMarket* eMarket;
  QList<Transaction*> transactions;
  QVariant buyStr;
  QVariant sellStr;
  QVariant sellIcon;
  QVariant buyIcon;
  QString dateFormat;
//  State state;

private: // QAbstractItemModel
  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
  virtual QModelIndex parent(const QModelIndex& child) const;
  virtual int rowCount(const QModelIndex& parent) const;
  virtual int columnCount(const QModelIndex& parent) const;
  virtual QVariant data(const QModelIndex& index, int role) const;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private: // Entity::Listener
  virtual void updatedEntitiy(Entity& entity);
};

class TransactionSortProxyModel2 : public QSortFilterProxyModel
{
public:
  TransactionSortProxyModel2(QObject* parent, TransactionModel2& transactionModel) : QSortFilterProxyModel(parent), transactionModel(transactionModel) {}

private:
  TransactionModel2& transactionModel;

  virtual bool lessThan(const QModelIndex& left, const QModelIndex& right) const
  {
    const TransactionModel2::Transaction* leftTransaction = transactionModel.getTransaction(left);
    const TransactionModel2::Transaction* rightTransaction = transactionModel.getTransaction(right);
    switch((TransactionModel2::Column)left.column())
    {
    case TransactionModel2::Column::date:
      return leftTransaction->date.msecsTo(rightTransaction->date) > 0;
    case TransactionModel2::Column::value:
      return leftTransaction->amount * leftTransaction->price < rightTransaction->amount * rightTransaction->price;
    case TransactionModel2::Column::amount:
      return leftTransaction->amount < rightTransaction->amount;
    case TransactionModel2::Column::price:
      return leftTransaction->price < rightTransaction->price;
    case TransactionModel2::Column::fee:
      return leftTransaction->fee < rightTransaction->fee;
    case TransactionModel2::Column::total:
      return leftTransaction->total < rightTransaction->total;
    default:
      break;
    }
    return QSortFilterProxyModel::lessThan(left, right);
  }
};
