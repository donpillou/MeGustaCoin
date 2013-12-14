
#include "stdafx.h"

OrderModel::OrderModel() : market(0), draftStr(tr("draft")), submittingStr(tr("submitting...")), openStr(tr("open")), cancelingStr(tr("canceling...")), 
canceledStr(tr("canceled")), closedStr(tr("closed")), buyStr(tr("buy")), sellStr(tr("sell")), nextDraftId(0)
{
  italicFont.setItalic(true);
}

OrderModel::~OrderModel()
{
  qDeleteAll(orders);
}

void OrderModel::setMarket(Market* market)
{
  this->market = market;
}

void OrderModel::reset()
{
  beginResetModel();
  market = 0;
  orders.clear();
  endResetModel();
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
    if(order->state == Order::State::open && openIt == openOrders.end())
    {
      order->state = Order::State::closed;
      QModelIndex index = createIndex(i, (int)Column::state, 0);
      emit dataChanged(index, index);
      openOrders.erase(openIt);
      continue;
    }
    if(openIt != openOrders.end())
    {
      *order = *openIt.value();
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

void OrderModel::updateOrder(const QString& id, const Order& newOrder)
{
  for(int i = 0, count = orders.size(); i < count; ++i)
  {
    Order* order = orders[i];
    if(order->id == id)
    {
      *order = newOrder;
      emit dataChanged(createIndex(i, (int)Column::first, 0), createIndex(i, (int)Column::last, 0));
      return;
    }
  }

  int oldOrderCount = orders.size();
  beginInsertRows(QModelIndex(), oldOrderCount, oldOrderCount);
  Order* order = new Order;
  *order = newOrder;
  orders.append(order);
  endInsertRows();
}

void OrderModel::setOrderState(const QString& id, Order::State state)
{
  for(int i = 0, count = orders.size(); i < count; ++i)
  {
    Order* order = orders[i];
    if(order->id == id)
    {
      order->state = state;
      QModelIndex index = createIndex(i, (int)Column::state, 0);
      emit dataChanged(index, index);
      return;
    }
  }
}

void OrderModel::setOrderNewAmount(const QString& id, double newAmount)
{
  for(int i = 0, count = orders.size(); i < count; ++i)
  {
    Order* order = orders[i];
    if(order->id == id)
    {
      order->newAmount = newAmount;
      QModelIndex index = createIndex(i, (int)Column::amount, 0);
      emit dataChanged(index, index);
      return;
    }
  }
}

int OrderModel::addOrder(Order::Type type, double price)
{
  int orderCount = orders.size();
  beginInsertRows(QModelIndex(), orderCount, orderCount);
  Order* newOrder = new Order;
  newOrder->id = QString("draft") + QString::number(nextDraftId++);
  newOrder->type = type;
  newOrder->state = Order::State::draft;
  newOrder->price = price;
  orders.append(newOrder);
  endInsertRows();
  return orderCount;
}

void OrderModel::removeOrder(const QModelIndex& index)
{
  int row = index.row();
  if(row < 0 || row >= orders.size())
    return;
  beginRemoveRows(QModelIndex(), row, row);
  delete orders[row];
  orders.removeAt(row);
  endRemoveRows();
}

const OrderModel::Order* OrderModel::getOrder(const QModelIndex& index) const
{
  int row = index.row();
  if(row < 0 || row >= orders.size())
    return 0;
  return orders[row];
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
  return (int)Column::last + 1;
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
    if(order.state == Order::State::open || order.state == Order::State::draft)
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

  if(role == Qt::TextAlignmentRole)
    switch((Column)index.column())
    {
    case Column::price:
    case Column::value:
    case Column::amount:
    case Column::total:
      return Qt::AlignRight;
    default:
      return Qt::AlignLeft;
    }

  int row = index.row();
  if(row < 0 || row >= orders.size())
    return QVariant();
  const Order& order = *orders[row];

  switch(role)
  {
  case Qt::FontRole:
    switch((Column)index.column())
    {
    case Column::price:
      if(order.newPrice != 0.)
        return italicFont;
      return QVariant();
    case Column::amount:
      if(order.newAmount != 0.)
        return italicFont;
      return QVariant();
    default:
      return QVariant();
    }
  case Qt::DisplayRole:
  case Qt::EditRole:
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
      if(role == Qt::EditRole)
        return order.newAmount != 0. ? order.newAmount : order.amount;
      return QString().sprintf("%.08f %s", order.amount, market->getCoinCurrency());
    case Column::price:
      if(role == Qt::EditRole)
        return order.newPrice != 0. ? order.newPrice : order.price;
      return QString().sprintf("%.02f %s", order.price, market->getMarketCurrency());
    case Column::value:
      return QString().sprintf("%.02f %s", order.amount * order.price, market->getMarketCurrency());
    case Column::state:
      switch(order.state)
      {
      case Order::State::draft:
        return draftStr;
      case Order::State::submitting:
        return submittingStr;
      case Order::State::open:
        return openStr;
      case Order::State::canceling:
        return cancelingStr;
      case Order::State::canceled:
        return canceledStr;
      case Order::State::closed:
        return closedStr;
      }
      break;
    case Column::total:
      return QString().sprintf("%+.02f %s", market->getOrderCharge(order.type == Order::Type::buy ? order.amount : -order.amount, order.price), market->getMarketCurrency());
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
        return tr("Amount");
      case Column::price:
        return tr("Price");
      case Column::value:
        return tr("Value");
      case Column::state:
        return tr("Status");
      case Column::total:
        return tr("Total");
    }
  }
  return QVariant();
}

bool OrderModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
  if (role != Qt::EditRole)
    return false;

  int row = index.row();
  if(row < 0 || row >= orders.size())
    return false;

  Order& order = *orders[row];
  if(order.state != OrderModel::Order::State::draft && order.state != OrderModel::Order::State::open)
    return false;

  switch((Column)index.column())
  {
  case Column::price:
    {
      double newPrice = value.toDouble();
      if(newPrice <= 0.)
        return false;
      if(order.state == OrderModel::Order::State::draft)
        order.price = newPrice;
      else if(newPrice != order.price)
      {
        order.newPrice = newPrice;
        emit orderEdited(index);
      }
      return true;
    }
  case Column::amount:
    {
      double newAmount = value.toDouble();
      if(newAmount <= 0.)
        return false;
      if(order.state == OrderModel::Order::State::draft)
        order.amount = newAmount;
      else if(newAmount != order.amount)
      {
        order.newAmount = value.toDouble();
        emit orderEdited(index);
      }
      return true;
    }
  }

  return false;
}
