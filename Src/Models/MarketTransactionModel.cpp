
#include "stdafx.h"

MarketTransactionModel::MarketTransactionModel(Entity::Manager& entityManager) :
  entityManager(entityManager),
  buyStr(tr("buy")), sellStr(tr("sell")),
  sellIcon(QIcon(":/Icons/money.png")), buyIcon(QIcon(":/Icons/bitcoin.png")),
  dateFormat(QLocale::system().dateTimeFormat(QLocale::ShortFormat))
{
  entityManager.registerListener<EUserBrokerTransaction>(*this);
  entityManager.registerListener<EConnection>(*this);

  eBrokerType = 0;
}

MarketTransactionModel::~MarketTransactionModel()
{
  entityManager.unregisterListener<EUserBrokerTransaction>(*this);
  entityManager.unregisterListener<EConnection>(*this);
}

QModelIndex MarketTransactionModel::index(int row, int column, const QModelIndex& parent) const
{
  if(hasIndex(row, column, parent))
    return createIndex(row, column, transactions.at(row));
  return QModelIndex();
}

QModelIndex MarketTransactionModel::parent(const QModelIndex& child) const
{
  return QModelIndex();
}

int MarketTransactionModel::rowCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : transactions.size();
}

int MarketTransactionModel::columnCount(const QModelIndex& parent) const
{
  return (int)Column::last + 1;
}

QVariant MarketTransactionModel::data(const QModelIndex& index, int role) const
{
  const EUserBrokerTransaction* eTransaction = (const EUserBrokerTransaction*)index.internalPointer();
  if(!eTransaction)
    return QVariant();

  switch(role)
  {
  case Qt::TextAlignmentRole:
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
  case Qt::DecorationRole:
    if((Column)index.column() == Column::type)
      switch(eTransaction->getType())
      {
      case EUserBrokerTransaction::Type::sell:
        return sellIcon;
      case EUserBrokerTransaction::Type::buy:
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
      case EUserBrokerTransaction::Type::buy:
        return buyStr;
      case EUserBrokerTransaction::Type::sell:
        return sellStr;
      default:
        break;
      }
    case Column::date:
      return eTransaction->getDate().toString(dateFormat);
    case Column::amount:
      return eBrokerType->formatAmount(eTransaction->getAmount());
    case Column::price:
      return eBrokerType->formatPrice(eTransaction->getPrice());
    case Column::value:
      return eBrokerType->formatPrice(eTransaction->getAmount() * eTransaction->getPrice());
    case Column::fee:
      return eBrokerType->formatPrice(eTransaction->getFee());
    case Column::total:
      return eTransaction->getType() == EUserBrokerTransaction::Type::sell ? (QString("+") + eBrokerType->formatPrice(eTransaction->getTotal())) : eBrokerType->formatPrice(-eTransaction->getTotal());
    }
  }
  return QVariant();
}

QVariant MarketTransactionModel::headerData(int section, Qt::Orientation orientation, int role) const
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
        return tr("Amount %1").arg(eBrokerType ? eBrokerType->getCommCurrency() : QString());
      case Column::price:
        return tr("Price %1").arg(eBrokerType ? eBrokerType->getBaseCurrency() : QString());
      case Column::value:
        return tr("Value %1").arg(eBrokerType ? eBrokerType->getBaseCurrency() : QString());
      case Column::fee:
        return tr("Fee %1").arg(eBrokerType ? eBrokerType->getBaseCurrency() : QString());
      case Column::total:
        return tr("Total %1").arg(eBrokerType ? eBrokerType->getBaseCurrency() : QString());
    }
  }
  return QVariant();
}

void MarketTransactionModel::addedEntity(Entity& entity)
{
  switch((EType)entity.getType())
  {
  case EType::userBrokerTransaction:
    {
      EUserBrokerTransaction* eTransaction = dynamic_cast<EUserBrokerTransaction*>(&entity);
      int index = transactions.size();
      beginInsertRows(QModelIndex(), index, index);
      transactions.append(eTransaction);
      endInsertRows();
      break;
    }
  case EType::connection:
    break;
  default:
    Q_ASSERT(false);
    break;
  }
}

void MarketTransactionModel::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
{
  switch((EType)oldEntity.getType())
  {
  case EType::userBrokerTransaction:
    {
      EUserBrokerTransaction* oldEBotMarketTransaction = dynamic_cast<EUserBrokerTransaction*>(&oldEntity);
      EUserBrokerTransaction* newEBotMarketTransaction = dynamic_cast<EUserBrokerTransaction*>(&newEntity);
      int index = transactions.indexOf(oldEBotMarketTransaction);
      transactions[index] = newEBotMarketTransaction; 
      QModelIndex leftModelIndex = createIndex(index, (int)Column::first, newEBotMarketTransaction);
      QModelIndex rightModelIndex = createIndex(index, (int)Column::last, newEBotMarketTransaction);
      emit dataChanged(leftModelIndex, rightModelIndex);
      break;
    }
  case EType::connection:
    {
      EConnection* eDataService = dynamic_cast<EConnection*>(&newEntity);
      EBrokerType* newBrokerType = 0;
      if(eDataService && eDataService->getSelectedBrokerId() != 0)
      {
        EUserBroker* eUserBroker = entityManager.getEntity<EUserBroker>(eDataService->getSelectedBrokerId());
        if(eUserBroker && eUserBroker->getBrokerTypeId() != 0)
          newBrokerType = entityManager.getEntity<EBrokerType>(eUserBroker->getBrokerTypeId());
      }
      if(newBrokerType != eBrokerType)
      {
        eBrokerType = newBrokerType;
        headerDataChanged(Qt::Horizontal, (int)Column::first, (int)Column::last);
      }
      break;
    }
  default:
    Q_ASSERT(false);
    break;
  }
}

void MarketTransactionModel::removedEntity(Entity& entity)
{
  switch((EType)entity.getType())
  {
  case EType::userBrokerTransaction:
    {
      EUserBrokerTransaction* eTransaction = dynamic_cast<EUserBrokerTransaction*>(&entity);
      int index = transactions.indexOf(eTransaction);
      beginRemoveRows(QModelIndex(), index, index);
      transactions.removeAt(index);
      endRemoveRows();
      break;
    }
  case EType::connection:
    break;
  default:
    Q_ASSERT(false);
    break;
  }
}

void MarketTransactionModel::removedAll(quint32 type)
{
  switch((EType)type)
  {
  case EType::userBrokerTransaction:
    if(!transactions.isEmpty())
    {
      emit beginResetModel();
      transactions.clear();
      emit endResetModel();
    }
    break;
  case EType::connection:
    break;
  default:
    Q_ASSERT(false);
    break;
  }
}
