
#include "stdafx.h"

OrderModel2::OrderModel2(Entity::Manager& entityManager) :
  entityManager(entityManager),
  draftStr(tr("draft")), submittingStr(tr("submitting...")), openStr(tr("open")), cancelingStr(tr("canceling...")), canceledStr(tr("canceled")), closedStr(tr("closed")), buyStr(tr("buy")), sellStr(tr("sell")), 
  sellIcon(QIcon(":/Icons/money.png")), buyIcon(QIcon(":/Icons/bitcoin.png")),
  dateFormat(QLocale::system().dateTimeFormat(QLocale::ShortFormat))
{
  entityManager.registerListener<EBotSessionOrder>(*this);

  eBotMarketAdapter = 0;
}

OrderModel2::~OrderModel2()
{
  entityManager.unregisterListener<EBotSessionOrder>(*this);
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
  const EBotSessionOrder* eOrder = (const EBotSessionOrder*)index.internalPointer();
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
    case Column::state:
      switch(eOrder->getState())
      {
      case EBotSessionOrder::State::draft:
        return draftStr;
      case EBotSessionOrder::State::submitting:
        return submittingStr;
      case EBotSessionOrder::State::open:
        return openStr;
      case EBotSessionOrder::State::canceling:
        return cancelingStr;
      case EBotSessionOrder::State::canceled:
        return canceledStr;
      case EBotSessionOrder::State::closed:
        return closedStr;
      }
      break;
    case Column::total:
        return eOrder->getTotal() > 0 ? (QString("+") + eBotMarketAdapter->formatPrice(eOrder->getTotal())) : eBotMarketAdapter->formatPrice(eOrder->getTotal());
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
        return tr("Amount %1").arg(eBotMarketAdapter ? eBotMarketAdapter->getCommCurrency() : QString());
      case Column::price:
        return tr("Price %1").arg(eBotMarketAdapter ? eBotMarketAdapter->getBaseCurrency() : QString());
      case Column::value:
        return tr("Value %1").arg(eBotMarketAdapter ? eBotMarketAdapter->getBaseCurrency() : QString());
      case Column::state:
        return tr("Status");
      case Column::total:
        return tr("Total %1").arg(eBotMarketAdapter ? eBotMarketAdapter->getBaseCurrency() : QString());
    }
  }
  return QVariant();
}

void OrderModel2::addedEntity(Entity& entity)
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

void OrderModel2::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
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

void OrderModel2::removedEntity(Entity& entity)
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

void OrderModel2::removedAll(quint32 type)
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
