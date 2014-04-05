
#include "stdafx.h"

OrderModel2::OrderModel2(Entity::Manager& entityManager) :
  entityManager(entityManager),
  draftStr(tr("draft")), submittingStr(tr("submitting...")), openStr(tr("open")), cancelingStr(tr("canceling...")), canceledStr(tr("canceled")), closedStr(tr("closed")), buyStr(tr("buy")), sellStr(tr("sell")), 
  sellIcon(QIcon(":/Icons/money.png")), buyIcon(QIcon(":/Icons/bitcoin.png")),
  dateFormat(QLocale::system().dateTimeFormat(QLocale::ShortFormat))
{
  entityManager.registerListener<EMarket>(*this);
  entityManager.registerListener<EOrder>(*this);

  eMarket = entityManager.getEntity<EMarket>(0);
  Q_ASSERT(eMarket);
}

OrderModel2::~OrderModel2()
{
  entityManager.unregisterListener<EMarket>(*this);
  entityManager.unregisterListener<EOrder>(*this);
}

//void OrderModel2::setState(State state)
//{
//  this->state = state;
//  emit changedState();
//}
//
//QString OrderModel2::getStateName() const
//{
//  switch(state)
//  {
//  case State::empty:
//    return QString();
//  case State::loading:
//    return tr("loading...");
//  case State::loaded:
//    return QString();
//  case State::error:
//    return tr("error");
//  }
//  Q_ASSERT(false);
//  return QString();
//}
//
//void OrderModel2::updateHeader()
//{
//  emit headerDataChanged(Qt::Horizontal, (int)Column::first, (int)Column::last);
//}

//void OrderModel2::setData(const QList<Market::Order>& updatedOrders)
//{
//  QHash<QString, const Market::Order*> openOrders;
//  foreach(const Market::Order& order, updatedOrders)
//    openOrders.insert(order.id, &order);
//
//  for(int i = 0, count = orders.size(); i < count; ++i)
//  {
//    Order* order = orders[i];
//    QHash<QString, const Market::Order*>::iterator openIt = openOrders.find(order->id);
//    if(order->state == Order::State::open && openIt == openOrders.end())
//    {
//      order->state = Order::State::closed;
//      QModelIndex index = createIndex(i, (int)Column::state, 0);
//      emit dataChanged(index, index);
//      openOrders.erase(openIt);
//      continue;
//    }
//    if(openIt != openOrders.end())
//    {
//      const Market::Order& updatedOrder = *openIt.value();
//      *order = updatedOrder;
//      emit dataChanged(createIndex(i, (int)Column::first, 0), createIndex(i, (int)Column::last, 0));
//      openOrders.erase(openIt);
//      continue;
//    }
//  }
//
//  if (openOrders.size() > 0)
//  {
//    int oldOrderCount = orders.size();
//    beginInsertRows(QModelIndex(), oldOrderCount, oldOrderCount + openOrders.size() - 1);
//
//    foreach(const Market::Order& newOrder, updatedOrders)
//    {
//      if(openOrders.contains(newOrder.id))
//      {
//        Order* order = new Order;
//        *order = newOrder;
//        orders.append(order);
//      }
//    }
//
//    endInsertRows();
//  }
//}

//void OrderModel2::addOrder(const Market::Order& order)
//{
//  updateOrder(order.id, order);
//}

//void OrderModel2::removeOrder(const QString& id)
//{
//  for(int i = 0, count = orders.size(); i < count; ++i)
//  {
//    Order* order = orders[i];
//    if(order->id == id)
//    {
//      beginRemoveRows(QModelIndex(), i, i);
//      delete order;
//      orders.removeAt(i);
//      endRemoveRows();
//      return;
//    }
//  }
//}

//void OrderModel2::updateOrder(const QString& id, const Market::Order& newOrder)
//{
//  for(int i = 0, count = orders.size(); i < count; ++i)
//  {
//    Order* order = orders[i];
//    if(order->id == id)
//    {
//      *order = newOrder;
//      if(newOrder.id.startsWith("draft_"))
//      {
//        order->state = Order::State::draft;
//        order->date = QDateTime();
//      }
//      emit dataChanged(createIndex(i, (int)Column::first, 0), createIndex(i, (int)Column::last, 0));
//      return;
//    }
//  }
//
//  int oldOrderCount = orders.size();
//  beginInsertRows(QModelIndex(), oldOrderCount, oldOrderCount);
//  Order* order = new Order;
//  *order = newOrder;
//  if(newOrder.id.startsWith("draft_"))
//  {
//    order->state = Order::State::draft;
//    order->date = QDateTime();
//  }
//  orders.append(order);
//  endInsertRows();
//}

//void OrderModel2::setOrderState(const QString& id, Order::State state)
//{
//  for(int i = 0, count = orders.size(); i < count; ++i)
//  {
//    Order* order = orders[i];
//    if(order->id == id)
//    {
//      order->state = state;
//      QModelIndex index = createIndex(i, (int)Column::state, 0);
//      emit dataChanged(index, index);
//      return;
//    }
//  }
//}

//void OrderModel2::setOrderNewAmount(const QString& id, double newAmount)
//{
//  for(int i = 0, count = orders.size(); i < count; ++i)
//  {
//    Order* order = orders[i];
//    if(order->id == id)
//    {
//      order->newAmount = newAmount;
//      QModelIndex index = createIndex(i, (int)Column::amount, 0);
//      emit dataChanged(index, index);
//      return;
//    }
//  }
//}

//QString OrderModel2::addOrderDraft(Order::Type type, double price)
//{
//  int orderCount = orders.size();
//  beginInsertRows(QModelIndex(), orderCount, orderCount);
//  Order* newOrder = new Order;
//  newOrder->id = QString("draft_") + QString::number(nextDraftId++);
//  newOrder->type = type;
//  newOrder->state = Order::State::draft;
//  newOrder->price = price;
//  orders.append(newOrder);
//  endInsertRows();
//  return newOrder->id;
//}

//QModelIndex OrderModel2::getOrderIndex(const QString& id) const
//{
//  for(int i = 0, count = orders.size(); i < count; ++i)
//  {
//    Order* order = orders[i];
//    if(order->id == id)
//      return createIndex(i, (int)Column::first, 0);
//  }
//  return QModelIndex();
//}

//const OrderModel2::Order* OrderModel2::getOrder(const QString& id) const
//{
//  for(int i = 0, count = orders.size(); i < count; ++i)
//  {
//    Order* order = orders[i];
//    if(order->id == id)
//      return order;
//  }
//  return 0;
//}

//const OrderModel2::Order* OrderModel2::getOrder(const QModelIndex& index) const
//{
//  int row = index.row();
//  if(row < 0 || row >= orders.size())
//    return 0;
//  return orders[row];
//}

//void OrderModel2::removeOrder(const QModelIndex& index)
//{
//  int row = index.row();
//  if(row < 0 || row >= orders.size())
//    return;
//  beginRemoveRows(QModelIndex(), row, row);
//  delete orders[row];
//  orders.removeAt(row);
//  endRemoveRows();
//}

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

//Qt::ItemFlags OrderModel2::flags(const QModelIndex &index) const
//{
//  if (!index.isValid())
//      return 0;
//  int row = index.row();
//  if(row >= 0 && row < orders.size())
//  {
//    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
//    const Order& order = *orders[row];
//    if(order.state == Order::State::open || order.state == Order::State::draft)
//    {
//      Column column = (Column)index.column();
//      if(column == Column::amount || column == Column::price)
//        flags |= Qt::ItemIsEditable;
//    }
//    return flags;
//  }
//  return 0;
//}

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
      return eMarket->formatAmount(eOrder->getAmount());
    case Column::price:
      return eMarket->formatPrice(eOrder->getPrice());
    case Column::value:
      return eMarket->formatPrice(eOrder->getAmount() * eOrder->getPrice());
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
        return eOrder->getTotal() > 0 ? (QString("+") + eMarket->formatPrice(eOrder->getTotal())) : eMarket->formatPrice(eOrder->getTotal());
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
        return tr("Amount %1").arg(eMarket->getCommCurrency());
      case Column::price:
        return tr("Price %1").arg(eMarket->getBaseCurrency());
      case Column::value:
        return tr("Value %1").arg(eMarket->getBaseCurrency());
      case Column::state:
        return tr("Status");
      case Column::total:
        return tr("Total %1").arg(eMarket->getBaseCurrency());
    }
  }
  return QVariant();
}

//bool OrderModel2::setData(const QModelIndex & index, const QVariant & value, int role)
//{
//  if (role != Qt::EditRole)
//    return false;
//
//  int row = index.row();
//  if(row < 0 || row >= orders.size())
//    return false;
//
//  Order& order = *orders[row];
//  if(order.state != OrderModel2::Order::State::draft && order.state != OrderModel2::Order::State::open)
//    return false;
//
//  switch((Column)index.column())
//  {
//  case Column::price:
//    {
//      double newPrice = value.toDouble();
//      if(newPrice <= 0.)
//        return false;
//      if(order.state == OrderModel2::Order::State::draft)
//      {
//        order.price = newPrice;
//        emit editedDraft(index);
//      }
//      else if(newPrice != order.price)
//      {
//        order.newPrice = newPrice;
//        emit editedOrder(index);
//      }
//      return true;
//    }
//  case Column::amount:
//    {
//      double newAmount = value.toDouble();
//      if(newAmount <= 0.)
//        return false;
//      if(order.state == OrderModel2::Order::State::draft)
//      {
//        order.amount = newAmount;
//        emit editedDraft(index);
//      }
//      else if(newAmount != order.amount)
//      {
//        order.newAmount = value.toDouble();
//        emit editedOrder(index);
//      }
//      return true;
//    }
//  default:
//    break;
//  }
//
//  return false;
//}

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

void OrderModel2::updatedEntitiy(Entity& entity)
{
  EOrder* eOrder = dynamic_cast<EOrder*>(&entity);
  if(eOrder)
  {
    int index = orders.indexOf(eOrder);
    QModelIndex leftModelIndex = createIndex(index, (int)Column::first, eOrder);
    QModelIndex rightModelIndex = createIndex(index, (int)Column::last, eOrder);
    emit dataChanged(leftModelIndex, rightModelIndex);
    return;
  }

  EMarket* eMarket = dynamic_cast<EMarket*>(&entity);
  if(eMarket)
  {
    Q_ASSERT(entity.getId() == 0);
    this->eMarket = eMarket;
    emit headerDataChanged(Qt::Horizontal, (int)Column::first, (int)Column::last);
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
