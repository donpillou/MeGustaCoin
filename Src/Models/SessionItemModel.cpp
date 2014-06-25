
#include "stdafx.h"

SessionItemModel::SessionItemModel(Entity::Manager& entityManager) :
  entityManager(entityManager),
  buyStr(tr("buy")), sellStr(tr("sell")),
  sellIcon(QIcon(":/Icons/money.png")), buyIcon(QIcon(":/Icons/bitcoin.png")),
  dateFormat(QLocale::system().dateTimeFormat(QLocale::ShortFormat))
{
  entityManager.registerListener<EBotSessionItem>(*this);

  eBotMarketAdapter = 0;
}

SessionItemModel::~SessionItemModel()
{
  entityManager.unregisterListener<EBotSessionItem>(*this);
}

QModelIndex SessionItemModel::index(int row, int column, const QModelIndex& parent) const
{
  if(hasIndex(row, column, parent))
    return createIndex(row, column, items.at(row));
  return QModelIndex();
}

QModelIndex SessionItemModel::parent(const QModelIndex& child) const
{
  return QModelIndex();
}

int SessionItemModel::rowCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : items.size();
}

int SessionItemModel::columnCount(const QModelIndex& parent) const
{
  return (int)Column::last + 1;
}

QVariant SessionItemModel::data(const QModelIndex& index, int role) const
{
  const EBotSessionItem* eItem = (const EBotSessionItem*)index.internalPointer();
  if(!eItem)
    return QVariant();

  if(role == Qt::TextAlignmentRole)
    switch((Column)index.column())
    {
    case Column::price:
    case Column::value:
    case Column::amount:
    case Column::flipPrice:
      return (int)Qt::AlignRight | (int)Qt::AlignVCenter;
    default:
      return (int)Qt::AlignLeft | (int)Qt::AlignVCenter;
    }

  switch(role)
  {
  case Qt::DecorationRole:
    switch((Column)index.column())
    {
    case Column::initialType:
      switch(eItem->getInitialType())
      {
      case EBotSessionItem::Type::sell:
        return sellIcon;
      case EBotSessionItem::Type::buy:
        return buyIcon;
      default:
        break;
      }
      break;
    case Column::currentType:
      switch(eItem->getCurrentType())
      {
      case EBotSessionItem::Type::sell:
        return sellIcon;
      case EBotSessionItem::Type::buy:
        return buyIcon;
      default:
        break;
      }
      break;
    default:
      break;
    }
    break;
  case Qt::DisplayRole:
    switch((Column)index.column())
    {
    case Column::initialType:
      switch(eItem->getInitialType())
      {
      case EBotSessionItem::Type::buy:
        return buyStr;
      case EBotSessionItem::Type::sell:
        return sellStr;
      default:
        break;
      }
      break;
    case Column::currentType:
      switch(eItem->getCurrentType())
      {
      case EBotSessionItem::Type::buy:
        return buyStr;
      case EBotSessionItem::Type::sell:
        return sellStr;
      default:
        break;
      }
      break;
    case Column::date:
      return eItem->getDate().toString(dateFormat);
    case Column::amount:
      return eBotMarketAdapter->formatAmount(eItem->getAmount());
    case Column::price:
      return eBotMarketAdapter->formatPrice(eItem->getPrice());
    case Column::value:
      return eBotMarketAdapter->formatPrice(eItem->getAmount() * eItem->getPrice());
    case Column::flipPrice:
      return eBotMarketAdapter->formatPrice(eItem->getFlipPrice());
    }
  }
  return QVariant();
}

QVariant SessionItemModel::headerData(int section, Qt::Orientation orientation, int role) const
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
      return Qt::AlignRight;
    default:
      return Qt::AlignLeft;
    }
  case Qt::DisplayRole:
    switch((Column)section)
    {
      case Column::initialType:
        return tr("Initial Type");
      case Column::currentType:
        return tr("Current Type");
      case Column::date:
        return tr("Date");
      case Column::amount:
        return tr("Amount %1").arg(eBotMarketAdapter ? eBotMarketAdapter->getCommCurrency() : QString());
      case Column::price:
        return tr("Price %1").arg(eBotMarketAdapter ? eBotMarketAdapter->getBaseCurrency() : QString());
      case Column::value:
        return tr("Value %1").arg(eBotMarketAdapter ? eBotMarketAdapter->getBaseCurrency() : QString());
    }
  }
  return QVariant();
}

void SessionItemModel::addedEntity(Entity& entity)
{
  EBotSessionItem* eItem = dynamic_cast<EBotSessionItem*>(&entity);
  if(eItem)
  {
    int index = items.size();
    beginInsertRows(QModelIndex(), index, index);
    items.append(eItem);
    endInsertRows();
    return;
  }
  Q_ASSERT(false);
}

void SessionItemModel::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
{
  EBotSessionItem* oldEBotSessionItem = dynamic_cast<EBotSessionItem*>(&oldEntity);
  if(oldEBotSessionItem)
  {
    EBotSessionItem* newEBotSessionItem = dynamic_cast<EBotSessionItem*>(&newEntity);
    int index = items.indexOf(oldEBotSessionItem);
    items[index] = newEBotSessionItem; 
    QModelIndex leftModelIndex = createIndex(index, (int)Column::first, newEBotSessionItem);
    QModelIndex rightModelIndex = createIndex(index, (int)Column::last, newEBotSessionItem);
    emit dataChanged(leftModelIndex, rightModelIndex);
    return;
  }
  Q_ASSERT(false);
}

void SessionItemModel::removedEntity(Entity& entity)
{
  EBotSessionItem* eItem = dynamic_cast<EBotSessionItem*>(&entity);
  if(eItem)
  {
    int index = items.indexOf(eItem);
    beginRemoveRows(QModelIndex(), index, index);
    items.removeAt(index);
    endRemoveRows();
    return;
  }
  Q_ASSERT(false);
}

void SessionItemModel::removedAll(quint32 type)
{
  if((EType)type == EType::botSessionItem)
  {
    emit beginResetModel();
    items.clear();
    emit endResetModel();
    return;
  }
  Q_ASSERT(false);
}
