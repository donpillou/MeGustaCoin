
#pragma once

class TransactionModel : public QAbstractItemModel
{
public:
  TransactionModel();
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

  void setMarket(Market* market);

  void reset();

  void setData(const QList<Transaction>& order);

  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
 
  const Transaction* getTransaction(const QModelIndex& index) const;

private:
  QList<Transaction*> transactions;
  Market* market;
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
};
