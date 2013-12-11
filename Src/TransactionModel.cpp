
#include "stdafx.h"

TransactionModel::TransactionModel() : buyStr(tr("buy")), sellStr(tr("sell"))
{
}

TransactionModel::~TransactionModel()
{
  qDeleteAll(transactions);
}

void TransactionModel::setCurrencies(const QString& market, const QString& coin)
{
  marketCurrency = market.toUtf8();
  coinCurrency = coin.toUtf8();
}

void TransactionModel::reset()
{
  beginResetModel();
  transactions.clear();
  endResetModel();
}

void TransactionModel::setData(const QList<Transaction>& updatedTransactions)
{
  QHash<QString, const Transaction*> newTransactions;
  foreach(const Transaction& transaction, updatedTransactions)
    newTransactions.insert(transaction.id, &transaction);

  for(int i = 0, count = transactions.size(); i < count; ++i)
  {
    Transaction* transaction = transactions[i];
    QHash<QString, const Transaction*>::iterator newIt = newTransactions.find(transaction->id);
    if(newIt == newTransactions.end())
    {
      beginRemoveRows(QModelIndex(), i, i);
      delete transaction;
      transactions.removeAt(i);
      endRemoveRows();
      --i;
      continue;
    }
    else
    {
      *transaction = *newIt.value();
      emit dataChanged(createIndex(i, (int)Column::first, 0), createIndex(i, (int)Column::last, 0));
      newTransactions.erase(newIt);
      continue;
    }
  }

  if (newTransactions.size() > 0)
  {
    int oldTransactionCount = transactions.size();
    beginInsertRows(QModelIndex(), oldTransactionCount, oldTransactionCount + newTransactions.size() - 1);

    foreach(const Transaction& transaction, updatedTransactions)
    {
      if(newTransactions.contains(transaction.id))
      {
        Transaction* newTransaction = new Transaction(transaction);
        transactions.append(newTransaction);
      }
    }

    endInsertRows();
  }
}

QModelIndex TransactionModel::index(int row, int column, const QModelIndex& parent) const
{
  return createIndex(row, column, 0);
}

QModelIndex TransactionModel::parent(const QModelIndex& child) const
{
  return QModelIndex();
}

int TransactionModel::rowCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : transactions.size();
}

int TransactionModel::columnCount(const QModelIndex& parent) const
{
  return (int)Column::last + 1;
}

QVariant TransactionModel::data(const QModelIndex& index, int role) const
{
  if(!index.isValid())
    return QVariant();

  if(role == Qt::TextAlignmentRole)
    switch((Column)index.column())
    {
    case Column::price:
    case Column::value:
    case Column::amount:
    case Column::fee:
    case Column::balance:
      return Qt::AlignRight;
    default:
      return Qt::AlignLeft;
    }

  int row = index.row();
  if(row < 0 || row >= transactions.size())
    return QVariant();
  const Transaction& transaction = *transactions[row];

  switch(role)
  {
  case Qt::DisplayRole:
    switch((Column)index.column())
    {
    case Column::type:
      switch(transaction.type)
      {
      case Transaction::Type::buy:
        return buyStr;
      case Transaction::Type::sell:
        return sellStr;
      }
    case Column::date:
      return transaction.date;
    case Column::amount:
      return QString().sprintf("%.08f %s", transaction.amount, coinCurrency.constData());
    case Column::price:
      return QString().sprintf("%.02f %s", transaction.price, marketCurrency.constData());
    case Column::value:
      return QString().sprintf("%.02f %s", transaction.amount * transaction.price, marketCurrency.constData());
    case Column::fee:
      return QString().sprintf("%.02f %s", transaction.fee, marketCurrency.constData());
    case Column::balance:
      return QString().sprintf("%+.02f %s", transaction.balanceChange, marketCurrency.constData());
    }
  }
  return QVariant();
}

QVariant TransactionModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if(orientation != Qt::Horizontal)
    return QVariant();
  switch(role)
  {
  case Qt::TextAlignmentRole:
    switch((Column)section)
    {
    case Column::price:
    case Column::value:
    case Column::amount:
    case Column::fee:
    case Column::balance:
      return Qt::AlignRight;
    default:
      return Qt::AlignLeft;
    }
  case Qt::DisplayRole:
    switch((Column)section)
    {
      case Column::type:
        return tr("Type");
      case Column::date:
        return tr("Date");
      case Column::amount:
        return tr("Amount");
      case Column::price:
        return tr("Price");
      case Column::value:
        return tr("Value");
      case Column::fee:
        return tr("Fee");
      case Column::balance:
        return tr("Total");
    }
  }
  return QVariant();
}
