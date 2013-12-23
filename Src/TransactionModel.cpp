
#include "stdafx.h"

TransactionModel::TransactionModel() : market(0), buyStr(tr("buy")), sellStr(tr("sell")),
sellIcon(QIcon(":/Icons/money.png")), buyIcon(QIcon(":/Icons/bitcoin.png"))
{
}

TransactionModel::~TransactionModel()
{
  qDeleteAll(transactions);
}

void TransactionModel::setMarket(Market* market)
{
  this->market = market;
  emit headerDataChanged(Qt::Horizontal, (int)Column::first, (int)Column::last);
}

void TransactionModel::reset()
{
  emit beginResetModel();
  market = 0;
  transactions.clear();
  emit endResetModel();
  emit headerDataChanged(Qt::Horizontal, (int)Column::first, (int)Column::last);
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
      count = transactions.size();
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
    case Column::total:
      return (int)Qt::AlignRight | (int)Qt::AlignVCenter;
    default:
      return (int)Qt::AlignLeft | (int)Qt::AlignVCenter;
    }

  int row = index.row();
  if(row < 0 || row >= transactions.size())
    return QVariant();
  const Transaction& transaction = *transactions[row];

  switch(role)
  {
  case Qt::DecorationRole:
    if((Column)index.column() == Column::type)
      switch(transaction.type)
      {
      case Transaction::Type::sell:
        return sellIcon;
      case Transaction::Type::buy:
        return buyIcon;
      }
    break;
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
      return market->formatAmount(transaction.amount);
      //return QString("%1 %2").arg(QLocale::system().toString(transaction.amount, 'f', 8), market->getCoinCurrency());
      //return QString().sprintf("%.08f %s", transaction.amount, market->getCoinCurrency());
    case Column::price:
      return market->formatPrice(transaction.price);
      //return QString("%1 %2").arg(QLocale::system().toString(transaction.price, 'f', 2), market->getMarketCurrency());
      //return QString().sprintf("%.02f %s", transaction.price, market->getMarketCurrency());
    case Column::value:
      return market->formatPrice(transaction.amount * transaction.price);
      //return QString("%1 %2").arg(QLocale::system().toString(transaction.amount * transaction.price, 'f', 2), market->getMarketCurrency());
      //return QString().sprintf("%.02f %s", transaction.amount * transaction.price, market->getMarketCurrency());
    case Column::fee:
      return market->formatPrice(transaction.fee);
      //return QString("%1 %2").arg(QLocale::system().toString(transaction.fee, 'f', 2), market->getMarketCurrency());
      //return QString().sprintf("%.02f %s", transaction.fee, market->getMarketCurrency());
    case Column::total:
      return transaction.balanceChange > 0 ? (QString("+") + market->formatPrice(transaction.balanceChange)) : market->formatPrice(transaction.balanceChange);
      //return QString(transaction.balanceChange > 0 ? "+%1 %2" : "%1 %2").arg(QLocale::system().toString(transaction.balanceChange, 'f', 2), market->getMarketCurrency());
      //return QString().sprintf("%+.02f %s", transaction.balanceChange, market->getMarketCurrency());
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
    case Column::total:
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
        return tr("Amount %1").arg(market ? market->getCoinCurrency() : "");
      case Column::price:
        return tr("Price %1").arg(market ? market->getMarketCurrency() : "");
      case Column::value:
        return tr("Value %1").arg(market ? market->getMarketCurrency() : "");
      case Column::fee:
        return tr("Fee %1").arg(market ? market->getMarketCurrency() : "");
      case Column::total:
        return tr("Total %1").arg(market ? market->getMarketCurrency() : "");
    }
  }
  return QVariant();
}
