
#include "stdafx.h"

BookModel::BookModel() {}

BookModel::ItemModel::ItemModel() : market(0) {}

BookModel::ItemModel::~ItemModel()
{
  qDeleteAll(items);
}

void BookModel::setMarket(Market* market)
{
  askModel.market = market;
  bidModel.market = market;
}

void BookModel::reset()
{
  askModel.market = 0;
  bidModel.market = 0;
  askModel.reset();
  bidModel.reset();
}

void BookModel::ItemModel::reset()
{
  beginResetModel();
  items.clear();
  endResetModel();
}

void BookModel::ItemModel::setData(const QList<Item>& newData)
{
  int i = 0;
  int oldCount = items.size();
  for(int count = qMin(oldCount, newData.size()); i < count; ++i)
  {
    *items[i] = newData[i];
  }
  if(i > 0)
    emit dataChanged(createIndex(0, (int)Column::first, 0), createIndex(i, (int)Column::last, 0));
  if(i < newData.size())
  { // add items
    beginInsertRows(QModelIndex(), oldCount, oldCount + (newData.size() - i) - 1);
    for(int count = newData.size(); i < count; ++i)
    {
      Item* newItem = new Item;
      *newItem = newData[i];
      items.push_back(newItem);
    }
    endInsertRows();
  }
  else if(items.size() > newData.size())
  { // remove items
    beginRemoveRows(QModelIndex(), i, items.size() - 1);
    for(int j = items.size() - 1; j >= i; --j)
    {
      delete items[j];
      items.removeAt(j);
    }
    endRemoveRows();
  }
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
  const Item& item = *items[row];

  switch(role)
  {
  case Qt::DisplayRole:
    switch((Column)index.column())
    {
    case Column::amount:
      return market->formatAmount(item.amount);
    case Column::price:
      return market->formatPrice(item.price);
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
        return tr("Amount");
      case Column::price:
        return tr("Price");
    }
  }
  return QVariant();
}
