
#pragma once

class TransactionModel : public QAbstractItemModel
{
  Q_OBJECT

public:
  TransactionModel(DataModel& dataModel);
  ~TransactionModel();

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

  void reset();

  void setData(const QList<Market::Transaction>& order);

  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
 
  const Transaction* getTransaction(const QModelIndex& index) const;

private:
  DataModel& dataModel;
  QList<Transaction*> transactions;
  QVariant buyStr;
  QVariant sellStr;
  QVariant sellIcon;
  QVariant buyIcon;
  QString dateFormat;

  virtual QModelIndex parent(const QModelIndex& child) const;
  virtual int rowCount(const QModelIndex& parent) const;
  virtual int columnCount(const QModelIndex& parent) const;
  virtual QVariant data(const QModelIndex& index, int role) const;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private slots:
  void updateHeader();
};
