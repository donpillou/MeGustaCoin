
#include "stdafx.h"

SessionTransactionModel::SessionTransactionModel(Entity::Manager& entityManager) :
  entityManager(entityManager),
  buyStr(tr("buy")), sellStr(tr("sell")),
  sellIcon(QIcon(":/Icons/money.png")), buyIcon(QIcon(":/Icons/bitcoin.png")),
  dateFormat(QLocale::system().dateTimeFormat(QLocale::ShortFormat))
{
  entityManager.registerListener<EBotSessionTransaction>(*this);
  entityManager.registerListener<EBotService>(*this);

  eBotMarketAdapter = 0;
}

SessionTransactionModel::~SessionTransactionModel()
{
  entityManager.unregisterListener<EBotSessionTransaction>(*this);
  entityManager.unregisterListener<EBotService>(*this);
}

QModelIndex SessionTransactionModel::index(int row, int column, const QModelIndex& parent) const
{
  if(hasIndex(row, column, parent))
    return createIndex(row, column, transactions.at(row));
  return QModelIndex();
}

QModelIndex SessionTransactionModel::parent(const QModelIndex& child) const
{
  return QModelIndex();
}

int SessionTransactionModel::rowCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : transactions.size();
}

int SessionTransactionModel::columnCount(const QModelIndex& parent) const
{
  return (int)Column::last + 1;
}

QVariant SessionTransactionModel::data(const QModelIndex& index, int role) const
{
  const EBotSessionTransaction* eTransaction = (const EBotSessionTransaction*)index.internalPointer();
  if(!eTransaction)
    return QVariant();

  switch(role)
  {
  case  Qt::TextAlignmentRole:
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
      case EBotSessionTransaction::Type::sell:
        return sellIcon;
      case EBotSessionTransaction::Type::buy:
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
      case EBotSessionTransaction::Type::buy:
        return buyStr;
      case EBotSessionTransaction::Type::sell:
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
      return eTransaction->getType() == EBotSessionTransaction::Type::sell ? (QString("+") + eBotMarketAdapter->formatPrice(eTransaction->getTotal())) : eBotMarketAdapter->formatPrice(-eTransaction->getTotal());
    }
  }
  return QVariant();
}

QVariant SessionTransactionModel::headerData(int section, Qt::Orientation orientation, int role) const
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

void SessionTransactionModel::addedEntity(Entity& entity)
{
  switch((EType)entity.getType())
  {
  case EType::botSessionTransaction:
    {
      EBotSessionTransaction* eTransaction = dynamic_cast<EBotSessionTransaction*>(&entity);
      int index = transactions.size();
      beginInsertRows(QModelIndex(), index, index);
      transactions.append(eTransaction);
      endInsertRows();
      break;
    }
  case EType::botService:
    break;
  default:
    Q_ASSERT(false);
    break;
  }
}

void SessionTransactionModel::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
{
  switch((EType)oldEntity.getType())
  {
  case EType::botSessionTransaction:
    {
      EBotSessionTransaction* oldEBotSessionTransaction = dynamic_cast<EBotSessionTransaction*>(&oldEntity);
      EBotSessionTransaction* newEBotSessionTransaction = dynamic_cast<EBotSessionTransaction*>(&newEntity);
      int index = transactions.indexOf(oldEBotSessionTransaction);
      transactions[index] = newEBotSessionTransaction; 
      QModelIndex leftModelIndex = createIndex(index, (int)Column::first, newEBotSessionTransaction);
      QModelIndex rightModelIndex = createIndex(index, (int)Column::last, newEBotSessionTransaction);
      emit dataChanged(leftModelIndex, rightModelIndex);
      break;
    }
  case EType::botService:
    {
      EBotService* eBotService = dynamic_cast<EBotService*>(&newEntity);
      EBotMarketAdapter* newMarketAdapter = 0;
      if(eBotService && eBotService->getSelectedSessionId() != 0)
      {
        EBotSession* eBotSession = entityManager.getEntity<EBotSession>(eBotService->getSelectedSessionId());
        if(eBotSession && eBotSession->getMarketId() != 0)
        {
          EBotMarket* eBotMarket = entityManager.getEntity<EBotMarket>(eBotSession->getMarketId());
          if(eBotMarket && eBotMarket->getMarketAdapterId() != 0)
            newMarketAdapter = entityManager.getEntity<EBotMarketAdapter>(eBotMarket->getMarketAdapterId());
        }
      }
      if(newMarketAdapter != eBotMarketAdapter)
      {
        eBotMarketAdapter = newMarketAdapter;
        headerDataChanged(Qt::Horizontal, (int)Column::first, (int)Column::last);
      }
      break;
    }
  default:
    Q_ASSERT(false);
    break;
  }
}

void SessionTransactionModel::removedEntity(Entity& entity)
{
  switch((EType)entity.getType())
  {
  case EType::botSessionTransaction:
    {
      EBotSessionTransaction* eTransaction = dynamic_cast<EBotSessionTransaction*>(&entity);
      int index = transactions.indexOf(eTransaction);
      beginRemoveRows(QModelIndex(), index, index);
      transactions.removeAt(index);
      endRemoveRows();
      break;
    }
  case EType::botService:
    break;
  default:
    Q_ASSERT(false);
    break;
  }
}

void SessionTransactionModel::removedAll(quint32 type)
{
  switch((EType)type)
  {
  case EType::botSessionTransaction:
    if(!transactions.isEmpty())
    {
      emit beginResetModel();
      transactions.clear();
      emit endResetModel();
    }
    break;
  case EType::botService:
    break;
  default:
    Q_ASSERT(false);
    break;
  }
}
