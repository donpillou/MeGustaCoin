
#include "stdafx.h"

OrderModel::OrderModel() : openStr(tr("open")), deletedStr(tr("deleted")), buyStr(tr("buy")), sellStr(tr("sell"))
{
  qDeleteAll(orders);
}

void OrderModel::setData(const QList<Order>& updatedOrders)
{
  QHash<QString, const Order*> openOrders;
  foreach(const Order& order, updatedOrders)
    openOrders.insert(order.id, &order);

  for(int i = 0, count = orders.size(); i < count; ++i)
  {
    Order* order = orders[i];
    QHash<QString, const Order*>::iterator openIt = openOrders.find(order->id);
    if(order->state != Order::State::deleted && openIt == openOrders.end())
    {
      order->state = Order::State::deleted;
      QModelIndex index = createIndex(i, (int)Column::state, 0);
      emit dataChanged(index, index);
      openOrders.erase(openIt);
      continue;
    }
    if(openIt != openOrders.end())
    {
      *orders[i] = *openIt.value();
      emit dataChanged(createIndex(i, (int)Column::first, 0), createIndex(i, (int)Column::last, 0));
      openOrders.erase(openIt);
      continue;
    }
  }

  if (openOrders.size() > 0)
  {
    int oldOrderCount = orders.size();
    beginInsertRows(QModelIndex(), oldOrderCount, oldOrderCount + openOrders.size() - 1);

    foreach(const Order& order, updatedOrders)
    {
      if(openOrders.contains(order.id))
      {
        Order* newOrder = new Order;
        *newOrder = order;
        orders.append(newOrder);
      }
    }

    endInsertRows();
  }
}

QModelIndex OrderModel::index(int row, int column, const QModelIndex& parent) const
{
  return createIndex(row, column, 0);
}

QModelIndex OrderModel::parent(const QModelIndex& child) const
{
  return QModelIndex();
}

int OrderModel::rowCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : orders.size();
}

int OrderModel::columnCount(const QModelIndex& parent) const
{
  return 6;
}

Qt::ItemFlags OrderModel::flags(const QModelIndex &index) const
{
  if (!index.isValid())
      return 0;
  int row = index.row();
  if(row >= 0 && row < orders.size())
  {
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    const Order& order = *orders[row];
    if(order.state == Order::State::open)
    {
      Column column = (Column)index.column();
      if(column == Column::amount || column == Column::price)
        flags |= Qt::ItemIsEditable;
    }
    return flags;
  }
  return 0;
}

QVariant OrderModel::data(const QModelIndex& index, int role) const
{
  if(!index.isValid())
    return QVariant();
  switch(role)
  {
    case Qt::TextAlignmentRole:
      switch((Column)index.column())
      {
      case Column::price:
      case Column::value:
      case Column::amount:
        return Qt::AlignRight;
      default:
        return Qt::AlignLeft;
      }
    case Qt::DisplayRole:
    case Qt::EditRole:
    {
      int row = index.row();
      if(row >= 0 && row < orders.size())
      {
        const Order& order = *orders[row];
        switch((Column)index.column())
        {
        case Column::type:
          switch(order.type)
          {
          case Order::Type::buy:
            return buyStr;
          case Order::Type::sell:
            return sellStr;
          }
        case Column::date:
          return order.date;
        case Column::amount:
          return order.amount;
        case Column::price:
          return order.price;
        case Column::value:
          return order.amount * order.price;
        case Column::state:
          switch(order.state)
          {
          case Order::State::open:
            return openStr;
          case Order::State::deleted:
            return deletedStr;
          }
        }
      }
    }
  }
  return QVariant();
}

QVariant OrderModel::headerData(int section, Qt::Orientation orientation, int role) const
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
      case Column::type:
        return tr("Type");
      case Column::date:
        return tr("Date");
      case Column::amount:
        return tr("Amount");
      case Column::price:
        return tr("Price");
      case Column::value:
        return tr("Value");
      case Column::state:
        return tr("Status");
    }
  }
  return QVariant();
}
