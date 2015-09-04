
#include "stdafx.h"

BotMarketModel::BotMarketModel(Entity::Manager& entityManager) : entityManager(entityManager),
  stoppedVar(tr("stopped")), startingVar(tr("starting")),runningVar(tr("running"))
{
  entityManager.registerListener<EBotMarket>(*this);
}

BotMarketModel::~BotMarketModel()
{
  entityManager.unregisterListener<EBotMarket>(*this);
}

QModelIndex BotMarketModel::index(int row, int column, const QModelIndex& parent) const
{
  if(hasIndex(row, column, parent))
    return createIndex(row, column, markets.at(row));
  return QModelIndex();
}

QModelIndex BotMarketModel::parent(const QModelIndex& child) const
{
  return QModelIndex();
}

int BotMarketModel::rowCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : markets.size();
}

int BotMarketModel::columnCount(const QModelIndex& parent) const
{
  return (int)Column::last + 1;
}

QVariant BotMarketModel::data(const QModelIndex& index, int role) const
{
  const EBotMarket* eBotMarket = (const EBotMarket*)index.internalPointer();
  if(!eBotMarket)
    return QVariant();

  switch(role)
  {
  case Qt::DisplayRole:
    switch((Column)index.column())
    {
    case Column::name:
      {
        EBotMarketAdapter* eBotMarketAdapter = entityManager.getEntity<EBotMarketAdapter>(eBotMarket->getBrokerTypeId());
        return eBotMarketAdapter ? eBotMarketAdapter->getName() : QVariant();
      }
    case Column::state:
      switch(eBotMarket->getState())
      {
      case EBotMarket::State::stopped:
        return stoppedVar;
      case EBotMarket::State::starting:
        return startingVar;
      case EBotMarket::State::running:
        return runningVar;
      }
      break;
    }
  }
  return QVariant();
}

QVariant BotMarketModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if(orientation != Qt::Horizontal)
    return QVariant();
  switch(role)
  {
  case Qt::DisplayRole:
    switch((Column)section)
    {
    case Column::name:
      return tr("Name");
    case Column::state:
      return tr("State");
    }
  }
  return QVariant();
}

void BotMarketModel::addedEntity(Entity& entity)
{
  EBotMarket* eBotMarket = dynamic_cast<EBotMarket*>(&entity);
  if(eBotMarket)
  {
    int index = markets.size();
    beginInsertRows(QModelIndex(), index, index);
    markets.append(eBotMarket);
    endInsertRows();
    return;
  }
  Q_ASSERT(false);
}

void BotMarketModel::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
{
  EBotMarket* oldeBotMarket = dynamic_cast<EBotMarket*>(&oldEntity);
  if(oldeBotMarket)
  {
    EBotMarket* neweBotMarket = dynamic_cast<EBotMarket*>(&newEntity);
    int index = markets.indexOf(oldeBotMarket);
    markets[index] = neweBotMarket;
    QModelIndex leftModelIndex = createIndex(index, (int)Column::first, oldeBotMarket);
    QModelIndex rightModelIndex = createIndex(index, (int)Column::last, oldeBotMarket);
    emit dataChanged(leftModelIndex, rightModelIndex);
    return;
  }
  Q_ASSERT(false);
}

void BotMarketModel::removedEntity(Entity& entity)
{
  EBotMarket* eBotMarket = dynamic_cast<EBotMarket*>(&entity);
  if(eBotMarket)
  {
    int index = markets.indexOf(eBotMarket);
    beginRemoveRows(QModelIndex(), index, index);
    markets.removeAt(index);
    endRemoveRows();
    return;
  }
  Q_ASSERT(false);
}

void BotMarketModel::removedAll(quint32 type)
{
  if((EType)type == EType::botMarket)
  {
    emit beginResetModel();
    markets.clear();
    emit endResetModel();
    return;
  }
  Q_ASSERT(false);
}
