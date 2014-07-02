
#include "stdafx.h"

SessionOrderModel::SessionOrderModel(Entity::Manager& entityManager) :
  entityManager(entityManager),
  draftStr(tr("draft")), submittingStr(tr("submitting...")), updatingStr(tr("updating...")), openStr(tr("open")), cancelingStr(tr("canceling...")), canceledStr(tr("canceled")), closedStr(tr("closed")), buyStr(tr("buy")), sellStr(tr("sell")), 
  sellIcon(QIcon(":/Icons/money.png")), buyIcon(QIcon(":/Icons/bitcoin.png")),
  dateFormat(QLocale::system().dateTimeFormat(QLocale::ShortFormat))
{
  entityManager.registerListener<EBotSessionOrder>(*this);

  eBotMarketAdapter = 0;
}

SessionOrderModel::~SessionOrderModel()
{
  entityManager.unregisterListener<EBotSessionOrder>(*this);
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
      return eBotMarketAdapter->formatAmount(eOrder->getAmount());
    case Column::price:
      return eBotMarketAdapter->formatPrice(eOrder->getPrice());
    case Column::value:
      return eBotMarketAdapter->formatPrice(eOrder->getAmount() * eOrder->getPrice());
    case Column::total:
        return eOrder->getTotal() > 0 ? (QString("+") + eBotMarketAdapter->formatPrice(eOrder->getTotal())) : eBotMarketAdapter->formatPrice(eOrder->getTotal());
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
        return tr("Amount %1").arg(eBotMarketAdapter ? eBotMarketAdapter->getCommCurrency() : QString());
      case Column::price:
        return tr("Price %1").arg(eBotMarketAdapter ? eBotMarketAdapter->getBaseCurrency() : QString());
      case Column::value:
        return tr("Value %1").arg(eBotMarketAdapter ? eBotMarketAdapter->getBaseCurrency() : QString());
      case Column::total:
        return tr("Total %1").arg(eBotMarketAdapter ? eBotMarketAdapter->getBaseCurrency() : QString());
    }
  }
  return QVariant();
}

void SessionOrderModel::addedEntity(Entity& entity)
{
  EBotSessionOrder* eOrder = dynamic_cast<EBotSessionOrder*>(&entity);
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

void SessionOrderModel::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
{
  EBotSessionOrder* oldEBotSessionOrder = dynamic_cast<EBotSessionOrder*>(&oldEntity);
  if(oldEBotSessionOrder)
  {
    EBotSessionOrder* newEBotSessionOrder = dynamic_cast<EBotSessionOrder*>(&newEntity);
    int index = orders.indexOf(oldEBotSessionOrder);
    orders[index] = newEBotSessionOrder; 
    QModelIndex leftModelIndex = createIndex(index, (int)Column::first, newEBotSessionOrder);
    QModelIndex rightModelIndex = createIndex(index, (int)Column::last, newEBotSessionOrder);
    emit dataChanged(leftModelIndex, rightModelIndex);
    return;
  }
  Q_ASSERT(false);
}

void SessionOrderModel::removedEntity(Entity& entity)
{
  EBotSessionOrder* eOrder = dynamic_cast<EBotSessionOrder*>(&entity);
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

void SessionOrderModel::removedAll(quint32 type)
{
  if((EType)type == EType::botSessionOrder)
  {
    emit beginResetModel();
    orders.clear();
    emit endResetModel();
    return;
  }
  Q_ASSERT(false);
}
