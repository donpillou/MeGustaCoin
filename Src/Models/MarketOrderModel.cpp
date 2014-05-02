
#include "stdafx.h"

MarketOrderModel::MarketOrderModel(Entity::Manager& entityManager) :
  entityManager(entityManager),
  draftStr(tr("draft")), submittingStr(tr("submitting...")), openStr(tr("open")), cancelingStr(tr("canceling...")), canceledStr(tr("canceled")), closedStr(tr("closed")), buyStr(tr("buy")), sellStr(tr("sell")), 
  sellIcon(QIcon(":/Icons/money.png")), buyIcon(QIcon(":/Icons/bitcoin.png")),
  dateFormat(QLocale::system().dateTimeFormat(QLocale::ShortFormat))
{
  entityManager.registerListener<EBotMarketOrder>(*this);

  eBotMarketAdapter = 0;
}

MarketOrderModel::~MarketOrderModel()
{
  entityManager.unregisterListener<EBotMarketOrder>(*this);
}

QModelIndex MarketOrderModel::createDraft(EBotMarketOrder::Type type, double price)
{
  quint32 id = entityManager.getNewEntityId<EBotMarketOrderDraft>();
  EBotMarketOrderDraft* eBotMarketOrderDraft = new EBotMarketOrderDraft(id, type, QDateTime::currentDateTime(), price);
  int index = orders.size();
  entityManager.delegateEntity(*eBotMarketOrderDraft);
  Q_ASSERT(orders[index] == eBotMarketOrderDraft);
  return createIndex(index, (int)Column::amount, eBotMarketOrderDraft);
}

QModelIndex MarketOrderModel::index(int row, int column, const QModelIndex& parent) const
{
  return createIndex(row, column, orders.at(row));
}

QModelIndex MarketOrderModel::parent(const QModelIndex& child) const
{
  return QModelIndex();
}

int MarketOrderModel::rowCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : orders.size();
}

int MarketOrderModel::columnCount(const QModelIndex& parent) const
{
  return (int)Column::last + 1;
}

QVariant MarketOrderModel::data(const QModelIndex& index, int role) const
{
  const EBotMarketOrder* eOrder = (const EBotMarketOrder*)index.internalPointer();
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
      case EBotMarketOrder::Type::sell:
        return sellIcon;
      case EBotMarketOrder::Type::buy:
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
      case EBotMarketOrder::Type::buy:
        return buyStr;
      case EBotMarketOrder::Type::sell:
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
      case EBotMarketOrder::State::draft:
        return draftStr;
      case EBotMarketOrder::State::submitting:
        return submittingStr;
      case EBotMarketOrder::State::open:
        return openStr;
      case EBotMarketOrder::State::canceling:
        return cancelingStr;
      case EBotMarketOrder::State::canceled:
        return canceledStr;
      case EBotMarketOrder::State::closed:
        return closedStr;
      }
      break;
    case Column::total:
        return eOrder->getTotal() > 0 ? (QString("+") + eBotMarketAdapter->formatPrice(eOrder->getTotal())) : eBotMarketAdapter->formatPrice(eOrder->getTotal());
    }
  }
  return QVariant();
}

Qt::ItemFlags MarketOrderModel::flags(const QModelIndex &index) const
{
  const EBotMarketOrder* eOrder = (const EBotMarketOrder*)index.internalPointer();
  if(!eOrder)
    return 0;

  Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  if(/*eOrder->getState() == EMarketOrder::State::open || */eOrder->getState() == EBotMarketOrder::State::draft)
  {
    Column column = (Column)index.column();
    if(column == Column::amount || column == Column::price)
      flags |= Qt::ItemIsEditable;
  }
  return flags;
}

bool MarketOrderModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
  if (role != Qt::EditRole)
    return false;

  EBotMarketOrder* eOrder = (EBotMarketOrder*)index.internalPointer();
  if(!eOrder)
    return false;

  if(eOrder->getState() != EBotMarketOrder::State::draft /* && eOrder->getState() != EBotMarketOrder::State::open */)
    return false;

  switch((Column)index.column())
  {
  case Column::price:
    {
      double newPrice = value.toDouble();
      if(newPrice <= 0. || newPrice == eOrder->getPrice())
        return false;
      if(eOrder->getState() == EBotMarketOrder::State::draft)
      {
        eOrder->setPrice(newPrice);
        EBotMarketBalance* eBotMarketBalance = entityManager.getEntity<EBotMarketBalance>(0);
        if(eBotMarketBalance)
          eOrder->setFee(qCeil(eOrder->getAmount() * eOrder->getPrice() * eBotMarketBalance->getFee()));
      }
      else // if(eOrder->getState() == EBotMarketOrder::State::open)
      {
        // todo: ??
        //order.newPrice = newPrice;
        //emit editedOrder(index);
      }
      return true;
    }
  case Column::amount:
    {
      double newAmount = value.toDouble();
      if(newAmount <= 0. || newAmount == eOrder->getAmount())
        return false;
      if(eOrder->getState() == EBotMarketOrder::State::draft)
      {
        eOrder->setAmount(newAmount);
        EBotMarketBalance* eBotMarketBalance = entityManager.getEntity<EBotMarketBalance>(0);
        if(eBotMarketBalance)
          eOrder->setFee(qCeil(eOrder->getAmount() * eOrder->getPrice() * eBotMarketBalance->getFee()));
      }
      else // if(eOrder->getState() == EBotMarketOrder::State::open)
      {
        // todo: ???
        //order.newAmount = value.toDouble();
        //emit editedOrder(index);
      }
      return true;
    }
  default:
    break;
  }

  return false;
}

QVariant MarketOrderModel::headerData(int section, Qt::Orientation orientation, int role) const
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

void MarketOrderModel::addedEntity(Entity& entity)
{
  EBotMarketOrder* eOrder = dynamic_cast<EBotMarketOrder*>(&entity);
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

void MarketOrderModel::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
{
  EBotMarketOrder* oldEBotMarketOrder = dynamic_cast<EBotMarketOrder*>(&oldEntity);
  if(oldEBotMarketOrder)
  {
    EBotMarketOrder* newEBotMarketOrder = dynamic_cast<EBotMarketOrder*>(&newEntity);
    int index = orders.indexOf(oldEBotMarketOrder);
    orders[index] = newEBotMarketOrder; 
    QModelIndex leftModelIndex = createIndex(index, (int)Column::first, newEBotMarketOrder);
    QModelIndex rightModelIndex = createIndex(index, (int)Column::last, newEBotMarketOrder);
    emit dataChanged(leftModelIndex, rightModelIndex);
    return;
  }
  Q_ASSERT(false);
}

void MarketOrderModel::removedEntity(Entity& entity)
{
  EBotMarketOrder* eOrder = dynamic_cast<EBotMarketOrder*>(&entity);
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

void MarketOrderModel::removedAll(quint32 type)
{
  if((EType)type == EType::botMarketOrder || 
     (EType)type == EType::botMarketOrderDraft)
  {
    if(!orders.isEmpty())
    {
      emit beginResetModel();
      orders.clear();
      emit endResetModel();
    }
    return;
  }
  Q_ASSERT(false);
}
