
#include "stdafx.h"

BotMarketModel::BotMarketModel(Entity::Manager& entityManager) : entityManager(entityManager),
  stoppedVar(tr("stopped")), startingVar(tr("starting")),runningVar(tr("running"))
{
  entityManager.registerListener<EUserBroker>(*this);
}

BotMarketModel::~BotMarketModel()
{
  entityManager.unregisterListener<EUserBroker>(*this);
}

QModelIndex BotMarketModel::index(int row, int column, const QModelIndex& parent) const
{
  if(hasIndex(row, column, parent))
    return createIndex(row, column, userBrokers.at(row));
  return QModelIndex();
}

QModelIndex BotMarketModel::parent(const QModelIndex& child) const
{
  return QModelIndex();
}

int BotMarketModel::rowCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : userBrokers.size();
}

int BotMarketModel::columnCount(const QModelIndex& parent) const
{
  return (int)Column::last + 1;
}

QVariant BotMarketModel::data(const QModelIndex& index, int role) const
{
  const EUserBroker* eUserBroker = (const EUserBroker*)index.internalPointer();
  if(!eUserBroker)
    return QVariant();

  switch(role)
  {
  case Qt::DisplayRole:
    switch((Column)index.column())
    {
    case Column::name:
      {
        EBrokerType* eBrokerType = entityManager.getEntity<EBrokerType>(eUserBroker->getBrokerTypeId());
        return eBrokerType ? eBrokerType->getName() : QVariant();
      }
    case Column::userName:
      return eUserBroker->getUserName();
    case Column::state:
      switch(eUserBroker->getState())
      {
      case EUserBroker::State::stopped:
        return stoppedVar;
      case EUserBroker::State::starting:
        return startingVar;
      case EUserBroker::State::running:
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
    case Column::userName:
      return tr("User");
    case Column::state:
      return tr("State");
    }
  }
  return QVariant();
}

void BotMarketModel::addedEntity(Entity& entity)
{
  EUserBroker* eUserBroker = dynamic_cast<EUserBroker*>(&entity);
  if(eUserBroker)
  {
    int index = userBrokers.size();
    beginInsertRows(QModelIndex(), index, index);
    userBrokers.append(eUserBroker);
    endInsertRows();
    return;
  }
  Q_ASSERT(false);
}

void BotMarketModel::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
{
  EUserBroker* oldUserBroker = dynamic_cast<EUserBroker*>(&oldEntity);
  if(oldUserBroker)
  {
    EUserBroker* newUserBroker = dynamic_cast<EUserBroker*>(&newEntity);
    int index = userBrokers.indexOf(oldUserBroker);
    userBrokers[index] = newUserBroker;
    QModelIndex leftModelIndex = createIndex(index, (int)Column::first, oldUserBroker);
    QModelIndex rightModelIndex = createIndex(index, (int)Column::last, oldUserBroker);
    emit dataChanged(leftModelIndex, rightModelIndex);
    return;
  }
  Q_ASSERT(false);
}

void BotMarketModel::removedEntity(Entity& entity)
{
  EUserBroker* eUserBroker = dynamic_cast<EUserBroker*>(&entity);
  if(eUserBroker)
  {
    int index = userBrokers.indexOf(eUserBroker);
    beginRemoveRows(QModelIndex(), index, index);
    userBrokers.removeAt(index);
    endRemoveRows();
    return;
  }
  Q_ASSERT(false);
}

void BotMarketModel::removedAll(quint32 type)
{
  if((EType)type == EType::userBroker)
  {
    emit beginResetModel();
    userBrokers.clear();
    emit endResetModel();
    return;
  }
  Q_ASSERT(false);
}
