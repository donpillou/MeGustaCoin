
#include "stdafx.h"

OrderModel2::OrderModel2(Entity::Manager& entityManager) :
  entityManager(entityManager),
  draftStr(tr("draft")), submittingStr(tr("submitting...")), openStr(tr("open")), cancelingStr(tr("canceling...")), canceledStr(tr("canceled")), closedStr(tr("closed")), buyStr(tr("buy")), sellStr(tr("sell")), 
  sellIcon(QIcon(":/Icons/money.png")), buyIcon(QIcon(":/Icons/bitcoin.png")),
  dateFormat(QLocale::system().dateTimeFormat(QLocale::ShortFormat))
{
  entityManager.registerListener<EOrder>(*this);

  eBotMarket = 0;
}

OrderModel2::~OrderModel2()
{
  entityManager.unregisterListener<EOrder>(*this);
}

QModelIndex OrderModel2::index(int row, int column, const QModelIndex& parent) const
{
  return createIndex(row, column, orders.at(row));
}

QModelIndex OrderModel2::parent(const QModelIndex& child) const
{
  return QModelIndex();
}

int OrderModel2::rowCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : orders.size();
}

int OrderModel2::columnCount(const QModelIndex& parent) const
{
  return (int)Column::last + 1;
}

QVariant OrderModel2::data(const QModelIndex& index, int role) const
{
  const EOrder* eOrder = (const EOrder*)index.internalPointer();
  if(!eOrder)
    return QVariant();

  if(role == Qt::TextAlignmentRole)
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

  switch(role)
  {
  case Qt::DecorationRole:
    if((Column)index.column() == Column::type)
      switch(eOrder->getType())
      {
      case EOrder::Type::sell:
        return sellIcon;
      case EOrder::Type::buy:
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
      case EOrder::Type::buy:
        return buyStr;
      case EOrder::Type::sell:
        return sellStr;
      default:
        break;
      }
      break;
    case Column::date:
      return eOrder->getDate().toString(dateFormat);
    case Column::amount:
      return eBotMarket->formatAmount(eOrder->getAmount());
    case Column::price:
      return eBotMarket->formatPrice(eOrder->getPrice());
    case Column::value:
      return eBotMarket->formatPrice(eOrder->getAmount() * eOrder->getPrice());
    case Column::state:
      switch(eOrder->getState())
      {
      case EOrder::State::draft:
        return draftStr;
      case EOrder::State::submitting:
        return submittingStr;
      case EOrder::State::open:
        return openStr;
      case EOrder::State::canceling:
        return cancelingStr;
      case EOrder::State::canceled:
        return canceledStr;
      case EOrder::State::closed:
        return closedStr;
      }
      break;
    case Column::total:
        return eOrder->getTotal() > 0 ? (QString("+") + eBotMarket->formatPrice(eOrder->getTotal())) : eBotMarket->formatPrice(eOrder->getTotal());
    }
  }
  return QVariant();
}

QVariant OrderModel2::headerData(int section, Qt::Orientation orientation, int role) const
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
        return tr("Amount %1").arg(eBotMarket ? eBotMarket->getCommCurrency() : QString());
      case Column::price:
        return tr("Price %1").arg(eBotMarket ? eBotMarket->getBaseCurrency() : QString());
      case Column::value:
        return tr("Value %1").arg(eBotMarket ? eBotMarket->getBaseCurrency() : QString());
      case Column::state:
        return tr("Status");
      case Column::total:
        return tr("Total %1").arg(eBotMarket ? eBotMarket->getBaseCurrency() : QString());
    }
  }
  return QVariant();
}

void OrderModel2::addedEntity(Entity& entity)
{
  EOrder* eOrder = dynamic_cast<EOrder*>(&entity);
  if(eOrder)
  {
    int index = orders.size();
    beginInsertRows(QModelIndex(), index, index);
    orders.append(eOrder);
    endInsertRows();
    return;
  }
  Q_ASSERT(false);
}

void OrderModel2::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
{
  EOrder* oldEOrder = dynamic_cast<EOrder*>(&oldEntity);
  if(oldEOrder)
  {
    EOrder* newEOrder = dynamic_cast<EOrder*>(&newEntity);
    int index = orders.indexOf(oldEOrder);
    orders[index] = newEOrder; 
    QModelIndex leftModelIndex = createIndex(index, (int)Column::first, newEOrder);
    QModelIndex rightModelIndex = createIndex(index, (int)Column::last, newEOrder);
    emit dataChanged(leftModelIndex, rightModelIndex);
    return;
  }
  Q_ASSERT(false);
}

void OrderModel2::removedEntity(Entity& entity)
{
  EOrder* eOrder = dynamic_cast<EOrder*>(&entity);
  if(eOrder)
  {
    int index = orders.indexOf(eOrder);
    beginRemoveRows(QModelIndex(), index, index);
    orders.removeAt(index);
    endRemoveRows();
    return;
  }
  Q_ASSERT(false);
}

void OrderModel2::removedAll(quint32 type)
{
  if((EType)type == EType::order)
  {
    emit beginResetModel();
    orders.clear();
    emit endResetModel();
    return;
  }
  Q_ASSERT(false);
}
