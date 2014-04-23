
#include "stdafx.h"

TransactionModel2::TransactionModel2(Entity::Manager& entityManager) :
  entityManager(entityManager),
  buyStr(tr("buy")), sellStr(tr("sell")),
  sellIcon(QIcon(":/Icons/money.png")), buyIcon(QIcon(":/Icons/bitcoin.png")),
  dateFormat(QLocale::system().dateTimeFormat(QLocale::ShortFormat))
{
  entityManager.registerListener<ETransaction>(*this);

  eBotMarketAdapter = 0;
}

TransactionModel2::~TransactionModel2()
{
  entityManager.unregisterListener<ETransaction>(*this);
}

QModelIndex TransactionModel2::index(int row, int column, const QModelIndex& parent) const
{
  return createIndex(row, column, transactions.at(row));
}

QModelIndex TransactionModel2::parent(const QModelIndex& child) const
{
  return QModelIndex();
}

int TransactionModel2::rowCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : transactions.size();
}

int TransactionModel2::columnCount(const QModelIndex& parent) const
{
  return (int)Column::last + 1;
}

QVariant TransactionModel2::data(const QModelIndex& index, int role) const
{
  const ETransaction* eTransaction = (const ETransaction*)index.internalPointer();
  if(!eTransaction)
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

  switch(role)
  {
  case Qt::DecorationRole:
    if((Column)index.column() == Column::type)
      switch(eTransaction->getType())
      {
      case ETransaction::Type::sell:
        return sellIcon;
      case ETransaction::Type::buy:
        return buyIcon;
      default:
        break;
      }
    break;
  case Qt::DisplayRole:
    switch((Column)index.column())
    {
    case Column::type:
      switch(eTransaction->getType())
      {
      case ETransaction::Type::buy:
        return buyStr;
      case ETransaction::Type::sell:
        return sellStr;
      default:
        break;
      }
    case Column::date:
      return eTransaction->getDate().toString(dateFormat);
    case Column::amount:
      return eBotMarketAdapter->formatAmount(eTransaction->getAmount());
    case Column::price:
      return eBotMarketAdapter->formatPrice(eTransaction->getPrice());
    case Column::value:
      return eBotMarketAdapter->formatPrice(eTransaction->getAmount() * eTransaction->getPrice());
    case Column::fee:
      return eBotMarketAdapter->formatPrice(eTransaction->getFee());
    case Column::total:
      return eTransaction->getTotal() > 0 ? (QString("+") + eBotMarketAdapter->formatPrice(eTransaction->getTotal())) : eBotMarketAdapter->formatPrice(eTransaction->getTotal());
    }
  }
  return QVariant();
}

QVariant TransactionModel2::headerData(int section, Qt::Orientation orientation, int role) const
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
        return tr("Amount %1").arg(eBotMarketAdapter ? eBotMarketAdapter->getCommCurrency() : QString());
      case Column::price:
        return tr("Price %1").arg(eBotMarketAdapter ? eBotMarketAdapter->getBaseCurrency() : QString());
      case Column::value:
        return tr("Value %1").arg(eBotMarketAdapter ? eBotMarketAdapter->getBaseCurrency() : QString());
      case Column::fee:
        return tr("Fee %1").arg(eBotMarketAdapter ? eBotMarketAdapter->getBaseCurrency() : QString());
      case Column::total:
        return tr("Total %1").arg(eBotMarketAdapter ? eBotMarketAdapter->getBaseCurrency() : QString());
    }
  }
  return QVariant();
}

void TransactionModel2::addedEntity(Entity& entity)
{
  ETransaction* eTransaction = dynamic_cast<ETransaction*>(&entity);
  if(eTransaction)
  {
    int index = transactions.size();
    beginInsertRows(QModelIndex(), index, index);
    transactions.append(eTransaction);
    endInsertRows();
    return;
  }
  Q_ASSERT(false);
}

void TransactionModel2::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
{
  ETransaction* oldETransaction = dynamic_cast<ETransaction*>(&oldEntity);
  if(oldETransaction)
  {
    ETransaction* newETransaction = dynamic_cast<ETransaction*>(&newEntity);
    int index = transactions.indexOf(oldETransaction);
    transactions[index] = newETransaction; 
    QModelIndex leftModelIndex = createIndex(index, (int)Column::first, newETransaction);
    QModelIndex rightModelIndex = createIndex(index, (int)Column::last, newETransaction);
    emit dataChanged(leftModelIndex, rightModelIndex);
    return;
  }
  Q_ASSERT(false);
}

void TransactionModel2::removedEntity(Entity& entity)
{
  ETransaction* eTransaction = dynamic_cast<ETransaction*>(&entity);
  if(eTransaction)
  {
    int index = transactions.indexOf(eTransaction);
    beginRemoveRows(QModelIndex(), index, index);
    transactions.removeAt(index);
    endRemoveRows();
    return;
  }
  Q_ASSERT(false);
}

void TransactionModel2::removedAll(quint32 type)
{
  if((EType)type == EType::transaction)
  {
    emit beginResetModel();
    transactions.clear();
    emit endResetModel();
    return;
  }
  Q_ASSERT(false);
}
