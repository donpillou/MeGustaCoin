
#include "stdafx.h"

SessionOrderModel::SessionOrderModel(Entity::Manager& entityManager) :
  entityManager(entityManager),
  draftStr(tr("draft")), submittingStr(tr("submitting...")), updatingStr(tr("updating...")), openStr(tr("open")), cancelingStr(tr("canceling...")), canceledStr(tr("canceled")), closedStr(tr("closed")), buyStr(tr("buy")), sellStr(tr("sell")), 
  sellIcon(QIcon(":/Icons/money.png")), buyIcon(QIcon(":/Icons/bitcoin.png")),
  dateFormat(QLocale::system().dateTimeFormat(QLocale::ShortFormat))
{
  entityManager.registerListener<EBotSessionOrder>(*this);
  entityManager.registerListener<EBotService>(*this);

  eBrokerType = 0;
}

SessionOrderModel::~SessionOrderModel()
{
  entityManager.unregisterListener<EBotSessionOrder>(*this);
  entityManager.unregisterListener<EBotService>(*this);
}

QModelIndex SessionOrderModel::index(int row, int column, const QModelIndex& parent) const
{
  if(hasIndex(row, column, parent))
    return createIndex(row, column, orders.at(row));
  return QModelIndex();
}

QModelIndex SessionOrderModel::parent(const QModelIndex& child) const
{
  return QModelIndex();
}

int SessionOrderModel::rowCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : orders.size();
}

int SessionOrderModel::columnCount(const QModelIndex& parent) const
{
  return (int)Column::last + 1;
}

QVariant SessionOrderModel::data(const QModelIndex& index, int role) const
{
  const EBotSessionOrder* eOrder = (const EBotSessionOrder*)index.internalPointer();
  if(!eOrder)
    return QVariant();

  switch(role)
  {
  case Qt::TextAlignmentRole:
    switch((Column)index.column())
    {
    case Column::price:
    case Column::value:
    case Column::amount:
    case Column::total:
      return (int)Qt::AlignRight | (int)Qt::AlignVCenter;
    default:
      return (int)Qt::AlignLeft | (int)Qt::AlignVCenter;
    }
  case Qt::DecorationRole:
    if((Column)index.column() == Column::type)
      switch(eOrder->getType())
      {
      case EBotSessionOrder::Type::sell:
        return sellIcon;
      case EBotSessionOrder::Type::buy:
        return buyIcon;
      default:
        break;
      }
    break;
  case Qt::DisplayRole:
    switch((Column)index.column())
    {
    case Column::type:
      switch(eOrder->getType())
      {
      case EBotSessionOrder::Type::buy:
        return buyStr;
      case EBotSessionOrder::Type::sell:
        return sellStr;
      default:
        break;
      }
      break;
    case Column::date:
      return eOrder->getDate().toString(dateFormat);
    case Column::amount:
      return eBrokerType->formatAmount(eOrder->getAmount());
    case Column::price:
      return eBrokerType->formatPrice(eOrder->getPrice());
    case Column::value:
      return eBrokerType->formatPrice(eOrder->getAmount() * eOrder->getPrice());
    case Column::total:
        return eOrder->getType() == EBotSessionOrder::Type::sell ? (QString("+") + eBrokerType->formatPrice(eOrder->getTotal())) : eBrokerType->formatPrice(-eOrder->getTotal());
    }
  }
  return QVariant();
}

QVariant SessionOrderModel::headerData(int section, Qt::Orientation orientation, int role) const
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
      case Column::total:
        return tr("Total %1").arg(eBrokerType ? eBrokerType->getBaseCurrency() : QString());
    }
  }
  return QVariant();
}

void SessionOrderModel::addedEntity(Entity& entity)
{
  switch((EType)entity.getType())
  {
  case EType::botSessionOrder:
    {
      EBotSessionOrder* eOrder = dynamic_cast<EBotSessionOrder*>(&entity);
      int index = orders.size();
      beginInsertRows(QModelIndex(), index, index);
      orders.append(eOrder);
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

void SessionOrderModel::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
{
  switch((EType)oldEntity.getType())
  {
  case EType::botSessionOrder:
    {
      EBotSessionOrder* oldEBotSessionOrder = dynamic_cast<EBotSessionOrder*>(&oldEntity);
      EBotSessionOrder* newEBotSessionOrder = dynamic_cast<EBotSessionOrder*>(&newEntity);
      int index = orders.indexOf(oldEBotSessionOrder);
      orders[index] = newEBotSessionOrder; 
      QModelIndex leftModelIndex = createIndex(index, (int)Column::first, newEBotSessionOrder);
      QModelIndex rightModelIndex = createIndex(index, (int)Column::last, newEBotSessionOrder);
      emit dataChanged(leftModelIndex, rightModelIndex);
      break;
    }
  case EType::botService:
    {
      EBotService* eBotService = dynamic_cast<EBotService*>(&newEntity);
      EBrokerType* newBrokerType = 0;
      if(eBotService && eBotService->getSelectedSessionId() != 0)
      {
        EBotSession* eBotSession = entityManager.getEntity<EBotSession>(eBotService->getSelectedSessionId());
        if(eBotSession && eBotSession->getBrokerId() != 0)
        {
          EBotMarket* eBotMarket = entityManager.getEntity<EBotMarket>(eBotSession->getBrokerId());
          if(eBotMarket && eBotMarket->getBrokerTypeId() != 0)
            newBrokerType = entityManager.getEntity<EBrokerType>(eBotMarket->getBrokerTypeId());
        }
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

void SessionOrderModel::removedEntity(Entity& entity)
{
  switch((EType)entity.getType())
  {
  case EType::botSessionOrder:
    {
      EBotSessionOrder* eOrder = dynamic_cast<EBotSessionOrder*>(&entity);
      int index = orders.indexOf(eOrder);
      beginRemoveRows(QModelIndex(), index, index);
      orders.removeAt(index);
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

void SessionOrderModel::removedAll(quint32 type)
{
  switch((EType)type)
  {
  case EType::botSessionOrder:
    if(!orders.isEmpty())
    {
      emit beginResetModel();
      orders.clear();
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
