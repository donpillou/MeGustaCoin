
#include "stdafx.h"

UserBrokerOrdersModel::UserBrokerOrdersModel(Entity::Manager& entityManager) :
  entityManager(entityManager),
  draftStr(tr("draft")), submittingStr(tr("submitting...")), updatingStr(tr("updating...")), openStr(tr("open")), cancelingStr(tr("canceling...")), canceledStr(tr("canceled")), closedStr(tr("closed")), buyStr(tr("buy")), sellStr(tr("sell")), errorStr(tr("error")),
  sellIcon(QIcon(":/Icons/money.png")), buyIcon(QIcon(":/Icons/bitcoin.png")),
  dateFormat(QLocale::system().dateTimeFormat(QLocale::ShortFormat))
{
  entityManager.registerListener<EUserBrokerOrder>(*this);
  entityManager.registerListener<EUserBrokerOrderDraft>(*this);
  entityManager.registerListener<EConnection>(*this);

  eBrokerType = 0;
}

UserBrokerOrdersModel::~UserBrokerOrdersModel()
{
  entityManager.unregisterListener<EUserBrokerOrder>(*this);
  entityManager.unregisterListener<EUserBrokerOrderDraft>(*this);
  entityManager.unregisterListener<EConnection>(*this);
}

QModelIndex UserBrokerOrdersModel::getDraftAmountIndex(EUserBrokerOrderDraft& draft)
{
  int index = orders.indexOf(&draft);
  return createIndex(index, (int)Column::amount, &draft);
}

QModelIndex UserBrokerOrdersModel::index(int row, int column, const QModelIndex& parent) const
{
  if(hasIndex(row, column, parent))
    return createIndex(row, column, orders.at(row));
  return QModelIndex();
}

QModelIndex UserBrokerOrdersModel::parent(const QModelIndex& child) const
{
  return QModelIndex();
}

int UserBrokerOrdersModel::rowCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : orders.size();
}

int UserBrokerOrdersModel::columnCount(const QModelIndex& parent) const
{
  return (int)Column::last + 1;
}

QVariant UserBrokerOrdersModel::data(const QModelIndex& index, int role) const
{
  const EUserBrokerOrder* eOrder = (const EUserBrokerOrder*)index.internalPointer();
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
      case EUserBrokerOrder::Type::sell:
        return sellIcon;
      case EUserBrokerOrder::Type::buy:
        return buyIcon;
      default:
        break;
      }
    break;
  case Qt::EditRole:
    switch((Column)index.column())
    {
    case Column::amount:
      return eOrder->getAmount();
    case Column::price:
      return eOrder->getPrice();
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
      case EUserBrokerOrder::Type::buy:
        return buyStr;
      case EUserBrokerOrder::Type::sell:
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
    case Column::state:
      switch(eOrder->getState())
      {
      case EUserBrokerOrder::State::draft:
        return draftStr;
      case EUserBrokerOrder::State::submitting:
        return submittingStr;
      case EUserBrokerOrder::State::updating:
        return updatingStr;
      case EUserBrokerOrder::State::open:
        return openStr;
      case EUserBrokerOrder::State::canceling:
        return cancelingStr;
      case EUserBrokerOrder::State::canceled:
        return canceledStr;
      case EUserBrokerOrder::State::closed:
        return closedStr;
      case EUserBrokerOrder::State::error:
        return errorStr;
      }
      break;
    case Column::total:
        return eOrder->getType() == EUserBrokerOrder::Type::sell ? (QString("+") + eBrokerType->formatPrice(eOrder->getTotal())) : eBrokerType->formatPrice(-eOrder->getTotal());
    }
  }
  return QVariant();
}

Qt::ItemFlags UserBrokerOrdersModel::flags(const QModelIndex &index) const
{
  const EUserBrokerOrderDraft* eOrder = (const EUserBrokerOrderDraft*)index.internalPointer();
  if(!eOrder)
    return 0;

  Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  if(eOrder->getState() == EUserBrokerOrder::State::open || eOrder->getState() == EUserBrokerOrder::State::draft)
  {
    Column column = (Column)index.column();
    if(column == Column::amount || column == Column::price)
      flags |= Qt::ItemIsEditable;
  }
  return flags;
}

bool UserBrokerOrdersModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
  if (role != Qt::EditRole)
    return false;

  EUserBrokerOrder* eOrder = (EUserBrokerOrder*)index.internalPointer();
  if(!eOrder)
    return false;

  if(eOrder->getState() != EUserBrokerOrder::State::draft && eOrder->getState() != EUserBrokerOrder::State::open)
    return false;

  switch((Column)index.column())
  {
  case Column::price:
    {
      double newPrice = value.toDouble();
      if(newPrice <= 0. || newPrice == eOrder->getPrice())
        return false;
      if(eOrder->getState() == EUserBrokerOrder::State::draft)
      {
        EUserBrokerBalance* eUserBrokerBalance = entityManager.getEntity<EUserBrokerBalance>(0);
        if(eUserBrokerBalance)
        {
          double total = eOrder->getType() == EUserBrokerOrder::Type::buy ?
            qCeil(eOrder->getAmount() * newPrice * (1. + eUserBrokerBalance->getFee()) * 100.) / 100. :
            qFloor(eOrder->getAmount() * newPrice * (1. - eUserBrokerBalance->getFee()) * 100.) / 100.;
          eOrder->setPrice(newPrice, total);
        }
      }
      else if(eOrder->getState() == EUserBrokerOrder::State::open)
      {
        if(eOrder->getPrice() != newPrice)
          emit editedOrderPrice(index, newPrice);
      }
      return true;
    }
  case Column::amount:
    {
      double newAmount = value.toDouble();
      if(newAmount <= 0. || newAmount == eOrder->getAmount())
        return false;
      if(eOrder->getState() == EUserBrokerOrder::State::draft)
      {
        EUserBrokerBalance* eUserBrokerBalance = entityManager.getEntity<EUserBrokerBalance>(0);
        if(eUserBrokerBalance)
        {
          double total = eOrder->getType() == EUserBrokerOrder::Type::buy ?
            qCeil(newAmount * eOrder->getPrice() * (1. + eUserBrokerBalance->getFee()) * 100.) / 100. :
            qFloor(newAmount * eOrder->getPrice() * (1. - eUserBrokerBalance->getFee()) * 100.) / 100.;
          eOrder->setAmount(newAmount, total);
        }
      }
      else if(eOrder->getState() == EUserBrokerOrder::State::open)
      {
        if(eOrder->getAmount() != newAmount)
          emit editedOrderAmount(index, newAmount);
      }
      return true;
    }
  default:
    break;
  }

  return false;
}

QVariant UserBrokerOrdersModel::headerData(int section, Qt::Orientation orientation, int role) const
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
      case Column::state:
        return tr("Status");
      case Column::total:
        return tr("Total %1").arg(eBrokerType ? eBrokerType->getBaseCurrency() : QString());
    }
  }
  return QVariant();
}

void UserBrokerOrdersModel::addedEntity(Entity& entity)
{
  switch((EType)entity.getType())
  {
  case EType::userBrokerOrder:
  case EType::userBrokerOrderDraft:
    {
      EUserBrokerOrder* eOrder = dynamic_cast<EUserBrokerOrder*>(&entity);
      int index = orders.size();
      beginInsertRows(QModelIndex(), index, index);
      orders.append(eOrder);
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

void UserBrokerOrdersModel::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
{
  switch((EType)oldEntity.getType())
  {
  case EType::userBrokerOrder:
  case EType::userBrokerOrderDraft:
    {
      EUserBrokerOrder* oldEBotMarketOrder = dynamic_cast<EUserBrokerOrder*>(&oldEntity);
      EUserBrokerOrder* newEBotMarketOrder = dynamic_cast<EUserBrokerOrder*>(&newEntity);
      int index = orders.indexOf(oldEBotMarketOrder);
      orders[index] = newEBotMarketOrder; 
      QModelIndex leftModelIndex = createIndex(index, (int)Column::first, newEBotMarketOrder);
      QModelIndex rightModelIndex = createIndex(index, (int)Column::last, newEBotMarketOrder);
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

void UserBrokerOrdersModel::addedEntity(Entity& entity, Entity& replacedEntity)
{
  updatedEntitiy(replacedEntity, entity);
}

void UserBrokerOrdersModel::removedEntity(Entity& entity)
{
  switch((EType)entity.getType())
  {
  case EType::userBrokerOrder:
  case EType::userBrokerOrderDraft:
    {
      EUserBrokerOrder* eOrder = dynamic_cast<EUserBrokerOrder*>(&entity);
      int index = orders.indexOf(eOrder);
      beginRemoveRows(QModelIndex(), index, index);
      orders.removeAt(index);
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

void UserBrokerOrdersModel::removedAll(quint32 type)
{
  switch((EType)type)
  {
  case EType::userBrokerOrder:
  case EType::userBrokerOrderDraft:
    if(!orders.isEmpty())
    {
      emit beginResetModel();
      orders.clear();
      emit endResetModel();
    }
    break;
  case EType::connection:
    break;
  default:
    Q_ASSERT(false);
  }
}
