
#include "stdafx.h"

BookModel::BookModel(PublicDataModel& publicDataModel) :
  askModel(publicDataModel), bidModel(publicDataModel),
  publicDataModel(publicDataModel),
  time(0)
{
  connect(&publicDataModel, SIGNAL(changedMarket()), this, SLOT(updateHeader()));
}

BookModel::ItemModel::ItemModel(PublicDataModel& publicDataModel) : publicDataModel(publicDataModel)
{
}

BookModel::ItemModel::~ItemModel()
{
  qDeleteAll(items);
}

void BookModel::updateHeader()
{
  askModel.updateHeader();
  bidModel.updateHeader();
}

void BookModel::setData(quint64 time, const QList<MarketStream::OrderBookEntry>& askItems, const QList<MarketStream::OrderBookEntry>& bidItems)
{
  if(time == this->time)
    return;

  this->time = time;
  askModel.setData(askItems);
  bidModel.setData(bidItems);
}

void BookModel::reset()
{
  askModel.reset();
  bidModel.reset();
}

void BookModel::ItemModel::updateHeader()
{
  emit headerDataChanged(Qt::Horizontal, (int)Column::first, (int)Column::last);
}

void BookModel::ItemModel::reset()
{
  emit beginResetModel();
  items.clear();
  emit endResetModel();
  emit headerDataChanged(Qt::Horizontal, (int)Column::first, (int)Column::last);
}

void BookModel::ItemModel::setData(const QList<MarketStream::OrderBookEntry>& newData)
{
  int oldCount = items.size();
  int newCount = qMin(newData.size(), 100);
  int destIndex = 0, srcIndex = newData.size() - newCount;
  for(int count = qMin(oldCount, newCount); destIndex < count; ++destIndex, ++srcIndex)
  {
    *items[destIndex] = newData[srcIndex];
  }
  if(destIndex > 0)
    emit dataChanged(createIndex(0, (int)Column::first, 0), createIndex(destIndex, (int)Column::last, 0));
  if(destIndex < newCount)
  { // add items
    beginInsertRows(QModelIndex(), oldCount, oldCount + (newCount - destIndex) - 1);
    for(int count = newCount; destIndex < count; ++destIndex, ++srcIndex)
    {
      MarketStream::OrderBookEntry* newItem = new MarketStream::OrderBookEntry;
      *newItem = newData[srcIndex];
      items.push_back(newItem);
    }
    endInsertRows();
  }
  else if(items.size() > newCount)
  { // remove items
    beginRemoveRows(QModelIndex(), destIndex, items.size() - 1);
    for(int j = items.size() - 1; j >= destIndex; --j)
    {
      delete items[j];
      items.removeAt(j);
    }
    endRemoveRows();
  }

  //graphModel.
}

QModelIndex BookModel::ItemModel::index(int row, int column, const QModelIndex& parent) const
{
  return createIndex(row, column, 0);
}

QModelIndex BookModel::ItemModel::parent(const QModelIndex& child) const
{
  return QModelIndex();
}

int BookModel::ItemModel::rowCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : items.size();
}

int BookModel::ItemModel::columnCount(const QModelIndex& parent) const
{
  return (int)Column::last + 1;
}

QVariant BookModel::ItemModel::data(const QModelIndex& index, int role) const
{
  if(!index.isValid())
    return QVariant();

  if(role == Qt::TextAlignmentRole)
    switch((Column)index.column())
    {
    case Column::price:
    case Column::amount:
      return (int)Qt::AlignRight | (int)Qt::AlignVCenter;
    default:
      return (int)Qt::AlignLeft | (int)Qt::AlignVCenter;
    }

  int row = index.row();
  if(row < 0 || row >= items.size())
    return QVariant();
  const MarketStream::OrderBookEntry& item = *items[row];

  switch(role)
  {
  case Qt::DisplayRole:
    switch((Column)index.column())
    {
    case Column::amount:
      return publicDataModel.formatAmount(item.amount);
    case Column::price:
      return publicDataModel.formatPrice(item.price);
    }
  }
  return QVariant();
}

QVariant BookModel::ItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if(orientation != Qt::Horizontal)
    return QVariant();
  switch(role)
  {
  case Qt::TextAlignmentRole:
    switch((Column)section)
    {
    case Column::price:
    case Column::amount:
      return Qt::AlignRight;
    default:
      return Qt::AlignLeft;
    }
  case Qt::DisplayRole:
    switch((Column)section)
    {
      case Column::amount:
        return tr("Amount %1").arg(publicDataModel.getCoinCurrency());
      case Column::price:
        return tr("Price %1").arg(publicDataModel.getMarketCurrency());
    }
  }
  return QVariant();
}
