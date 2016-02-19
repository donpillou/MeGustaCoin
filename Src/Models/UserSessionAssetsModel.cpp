
#include "stdafx.h"

UserSessionAssetsModel::UserSessionAssetsModel(Entity::Manager& entityManager) :
  entityManager(entityManager), draftStr(tr("draft")),
  submittingStr("submitting..."),
  buyStr(tr("buy")), sellStr(tr("sell")),
  buyingStr(tr("buying...")), sellingStr(tr("selling...")),
  sellIcon(QIcon(":/Icons/money.png")), buyIcon(QIcon(":/Icons/bitcoin.png")),
  dateFormat(QLocale::system().dateTimeFormat(QLocale::ShortFormat))
{
  entityManager.registerListener<EUserSessionAsset>(*this);
  entityManager.registerListener<EUserSessionAssetDraft>(*this);
  entityManager.registerListener<EConnection>(*this);

  eBrokerType = 0;
}

UserSessionAssetsModel::~UserSessionAssetsModel()
{
  entityManager.unregisterListener<EUserSessionAsset>(*this);
  entityManager.unregisterListener<EUserSessionAssetDraft>(*this);
  entityManager.unregisterListener<EConnection>(*this);
}

QModelIndex UserSessionAssetsModel::getDraftAmountIndex(EUserSessionAssetDraft& draft)
{
  int index = items.indexOf(&draft);
  return createIndex(index, (int)(draft.getType() == EUserSessionAsset::Type::buy ? Column::balanceBase : Column::balanceComm), &draft);
}

QModelIndex UserSessionAssetsModel::index(int row, int column, const QModelIndex& parent) const
{
  if(hasIndex(row, column, parent))
    return createIndex(row, column, items.at(row));
  return QModelIndex();
}

QModelIndex UserSessionAssetsModel::parent(const QModelIndex& child) const
{
  return QModelIndex();
}

int UserSessionAssetsModel::rowCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : items.size();
}

int UserSessionAssetsModel::columnCount(const QModelIndex& parent) const
{
  return (int)Column::last + 1;
}

QVariant UserSessionAssetsModel::data(const QModelIndex& index, int role) const
{
  const EUserSessionAsset* eItem = (const EUserSessionAsset*)index.internalPointer();
  if(!eItem)
    return QVariant();

  switch(role)
  {
  case Qt::TextAlignmentRole:
    switch((Column)index.column())
    {
    case Column::price:
    case Column::investBase:
    case Column::investComm:
    case Column::balanceBase:
    case Column::balanceComm:
    case Column::profitablePrice:
    case Column::flipPrice:
      return (int)Qt::AlignRight | (int)Qt::AlignVCenter;
    default:
      return (int)Qt::AlignLeft | (int)Qt::AlignVCenter;
    }
  case Qt::DecorationRole:
    switch((Column)index.column())
    {
    case Column::type:
      switch(eItem->getType())
      {
      case EUserSessionAsset::Type::sell:
        return sellIcon;
      case EUserSessionAsset::Type::buy:
        return buyIcon;
      default:
        break;
      }
      break;
    case Column::state:
      switch(eItem->getState())
      {
      case EUserSessionAsset::State::waitSell:
      case EUserSessionAsset::State::selling:
        return sellIcon;
      case EUserSessionAsset::State::waitBuy:
      case EUserSessionAsset::State::buying:
        return buyIcon;
      case EUserSessionAsset::State::draft:
        return eItem->getType() == EUserSessionAsset::Type::sell ? sellIcon : buyIcon;
      default:
        break;
      }
      break;
    default:
      break;
    }
    break;
  case Qt::EditRole:
    switch((Column)index.column())
    {
    case Column::balanceBase:
      return eItem->getBalanceBase();
    case Column::balanceComm:
      return eItem->getBalanceComm();
    case Column::flipPrice:
      return eItem->getFlipPrice();
    default:
      break;
    }
    break;
  case Qt::DisplayRole:
    switch((Column)index.column())
    {
    case Column::type:
      switch(eItem->getType())
      {
      case EUserSessionAsset::Type::buy:
        return buyStr;
      case EUserSessionAsset::Type::sell:
        return sellStr;
      default:
        break;
      }
      break;
    case Column::state:
      switch(eItem->getState())
      {
      case EUserSessionAsset::State::submitting:
        return submittingStr;
      case EUserSessionAsset::State::waitBuy:
        return buyStr;
      case EUserSessionAsset::State::buying:
        return buyingStr;
      case EUserSessionAsset::State::waitSell:
        return sellStr;
      case EUserSessionAsset::State::selling:
        return sellingStr;
      case EUserSessionAsset::State::draft:
        return draftStr;
      default:
        break;
      }
      break;
    case Column::date:
      return eItem->getDate().toString(dateFormat);
    case Column::investBase:
      return eBrokerType->formatPrice(eItem->getInvestBase());
    case Column::investComm:
      return eBrokerType->formatAmount(eItem->getInvestComm());
    case Column::balanceBase:
      return eBrokerType->formatPrice(eItem->getBalanceBase());
    case Column::balanceComm:
      return eBrokerType->formatAmount(eItem->getBalanceComm());
    case Column::price:
      return eItem->getPrice() == 0 ? QString() : eBrokerType->formatPrice(eItem->getPrice());
    case Column::profitablePrice:
      return eItem->getPrice() == 0 ? QString() : eBrokerType->formatPrice(eItem->getProfitablePrice());
    case Column::flipPrice:
      return eBrokerType->formatPrice(eItem->getFlipPrice());
    }
  }
  return QVariant();
}

Qt::ItemFlags UserSessionAssetsModel::flags(const QModelIndex &index) const
{
  const EUserSessionAsset* eItem = (const EUserSessionAsset*)index.internalPointer();
  if(!eItem)
    return 0;

  Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  switch((EUserSessionAsset::State)eItem->getState())
  {
  case EUserSessionAsset::State::draft:
    {
      Column column = (Column)index.column();
      if(column == (eItem->getType() == EUserSessionAsset::Type::buy ? Column::balanceBase : Column::balanceComm) || column == Column::flipPrice)
        flags |= Qt::ItemIsEditable;
    }
    break;
  case EUserSessionAsset::State::waitBuy:
  case EUserSessionAsset::State::waitSell:
    {
      Column column = (Column)index.column();
      if(column == Column::flipPrice)
        flags |= Qt::ItemIsEditable;
    }
    break;
  default:
    break;
  }
  return flags;
}

bool UserSessionAssetsModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
  if (role != Qt::EditRole)
    return false;

  EUserSessionAsset* eItem = (EUserSessionAsset*)index.internalPointer();
  if(!eItem)
    return false;

  switch(eItem->getState())
  {
  case EUserSessionAsset::State::draft:
    switch((Column)index.column())
    {
    case Column::flipPrice:
      {
        double newPrice = value.toDouble();
        if(newPrice <= 0. || newPrice == eItem->getPrice())
          return false;
        if(eItem->getState() == EUserSessionAsset::State::draft)
          eItem->setFlipPrice(newPrice);
        return true;
      }
    case Column::balanceBase:
      if(eItem->getType() == EUserSessionAsset::Type::buy)
      {
        double newBalanceBase = value.toDouble();
        if(newBalanceBase <= 0. || newBalanceBase == eItem->getBalanceBase())
          return false;
        if(eItem->getState() == EUserSessionAsset::State::draft)
          eItem->setBalanceBase(newBalanceBase);
        return true;
      }
    case Column::balanceComm:
      if(eItem->getType() == EUserSessionAsset::Type::sell)
      {
        double newBalanceComm = value.toDouble();
        if(newBalanceComm <= 0. || newBalanceComm == eItem->getBalanceComm())
          return false;
        if(eItem->getState() == EUserSessionAsset::State::draft)
          eItem->setBalanceComm(newBalanceComm);
        return true;
      }
    default:
      break;
    }
    break;
  case EUserSessionAsset::State::waitBuy:
  case EUserSessionAsset::State::waitSell:
    if((Column)index.column() == Column::flipPrice)
    {
      double newFlipPrice = value.toDouble();
      if(newFlipPrice != eItem->getFlipPrice())
        emit editedItemFlipPrice(index, newFlipPrice);
    }
    break;
  default:
    break;
  }

  return false;
}

QVariant UserSessionAssetsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if(orientation != Qt::Horizontal)
    return QVariant();
  switch(role)
  {
  case Qt::TextAlignmentRole:
    switch((Column)section)
    {
    case Column::investBase:
    case Column::investComm:
    case Column::balanceBase:
    case Column::balanceComm:
    case Column::price:
    case Column::profitablePrice:
    case Column::flipPrice:
      return Qt::AlignRight;
    default:
      return Qt::AlignLeft;
    }
  case Qt::DisplayRole:
    switch((Column)section)
    {
      case Column::type:
        return tr("Type");
      case Column::state:
        return tr("State");
      case Column::date:
        return tr("Date");
      case Column::price:
        return tr("Last Price %1").arg(eBrokerType ? eBrokerType->getBaseCurrency() : QString());
      case Column::investBase:
        return tr("Input %1").arg(eBrokerType ? eBrokerType->getBaseCurrency() : QString());
      case Column::investComm:
        return tr("Input %1").arg(eBrokerType ? eBrokerType->getCommCurrency() : QString());
      case Column::balanceBase:
        return tr("Balance %1").arg(eBrokerType ? eBrokerType->getBaseCurrency() : QString());
      case Column::balanceComm:
        return tr("Balance %1").arg(eBrokerType ? eBrokerType->getCommCurrency() : QString());
      case Column::profitablePrice:
        return tr("Min Price %1").arg(eBrokerType ? eBrokerType->getBaseCurrency() : QString());
      case Column::flipPrice:
        return tr("Next Price %1").arg(eBrokerType ? eBrokerType->getBaseCurrency() : QString());
    }
  }
  return QVariant();
}

void UserSessionAssetsModel::addedEntity(Entity& entity)
{
  switch((EType)entity.getType())
  {
  case EType::userSessionItem:
  case EType::userSessionItemDraft:
    {
      EUserSessionAsset* eItem = dynamic_cast<EUserSessionAsset*>(&entity);
      int index = items.size();
      beginInsertRows(QModelIndex(), index, index);
      items.append(eItem);
      endInsertRows();
      break;
    }
  case EType::connection:
    break;
  default:
    Q_ASSERT(false);
    break;
  }
}

void UserSessionAssetsModel::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
{
  switch((EType)oldEntity.getType())
  {
  case EType::userSessionItem:
  case EType::userSessionItemDraft:
    {
      EUserSessionAsset* oldEBotSessionItem = dynamic_cast<EUserSessionAsset*>(&oldEntity);
      EUserSessionAsset* newEBotSessionItem = dynamic_cast<EUserSessionAsset*>(&newEntity);
      int index = items.indexOf(oldEBotSessionItem);
      items[index] = newEBotSessionItem; 
      QModelIndex leftModelIndex = createIndex(index, (int)Column::first, newEBotSessionItem);
      QModelIndex rightModelIndex = createIndex(index, (int)Column::last, newEBotSessionItem);
      emit dataChanged(leftModelIndex, rightModelIndex);
      break;
    }
  case EType::connection:
    {
      EConnection* eDataService = dynamic_cast<EConnection*>(&newEntity);
      EBrokerType* newBrokerType = 0;
      if(eDataService && eDataService->getSelectedSessionId() != 0)
      {
        EUserSession* eSession = entityManager.getEntity<EUserSession>(eDataService->getSelectedSessionId());
        if(eSession && eSession->getBrokerId() != 0)
        {
          EUserBroker* eUserBroker = entityManager.getEntity<EUserBroker>(eSession->getBrokerId());
          if(eUserBroker && eUserBroker->getBrokerTypeId() != 0)
            newBrokerType = entityManager.getEntity<EBrokerType>(eUserBroker->getBrokerTypeId());
        }
      }
      if(newBrokerType != eBrokerType)
      {
        eBrokerType = newBrokerType;
        headerDataChanged(Qt::Horizontal, (int)Column::first, (int)Column::last);
      }
      break;
    }
  default:
    Q_ASSERT(false);
    break;
  }
}

void UserSessionAssetsModel::addedEntity(Entity& entity, Entity& replacedEntity)
{
  updatedEntitiy(replacedEntity, entity);
}

void UserSessionAssetsModel::removedEntity(Entity& entity)
{
  switch((EType)entity.getType())
  {
  case EType::userSessionItem:
  case EType::userSessionItemDraft:
    {
      EUserSessionAsset* eItem = dynamic_cast<EUserSessionAsset*>(&entity);
      int index = items.indexOf(eItem);
      beginRemoveRows(QModelIndex(), index, index);
      items.removeAt(index);
      endRemoveRows();
      break;
    }
  case EType::connection:
    break;
  default:
    Q_ASSERT(false);
    break;
  }
}

void UserSessionAssetsModel::removedAll(quint32 type)
{
  switch((EType)type)
  {
  case EType::userSessionItem:
  case EType::userSessionItemDraft:
    if(!items.isEmpty())
    {
      emit beginResetModel();
      items.clear();
      emit endResetModel();
    }
    break;;
  case EType::connection:
    break;
  default:
    Q_ASSERT(false);
    break;
  }
}
