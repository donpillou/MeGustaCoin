
#include "stdafx.h"

BotSessionModel::BotSessionModel(Entity::Manager& entityManager) : entityManager(entityManager),
  stoppedVar(tr("stopped")), activeVar(tr("active")), simulatingVar(tr("simulating"))
{
  entityManager.registerListener<EBotSession>(*this);
}

BotSessionModel::~BotSessionModel()
{
  entityManager.unregisterListener<EBotSession>(*this);
}

QModelIndex BotSessionModel::index(int row, int column, const QModelIndex& parent) const
{
  if(row < 0)
    return QModelIndex();
  return createIndex(row, column, sessions.at(row));
}

QModelIndex BotSessionModel::parent(const QModelIndex& child) const
{
  return QModelIndex();
}

int BotSessionModel::rowCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : sessions.size();
}

int BotSessionModel::columnCount(const QModelIndex& parent) const
{
  return (int)Column::last + 1;
}

QVariant BotSessionModel::data(const QModelIndex& index, int role) const
{
  const EBotSession* eSession = (const EBotSession*)index.internalPointer();
  if(!eSession)
    return QVariant();

  switch(role)
  {
  case Qt::DisplayRole:
    switch((Column)index.column())
    {
    case Column::name:
      return eSession->getName();
    case Column::engine:
    {
      EBotEngine* eEngine = entityManager.getEntity<EBotEngine>(eSession->getEngineId());
      return eEngine ? eEngine->getName() : QVariant();
    }
    case Column::market:
    {
      EBotMarketAdapter* eBotMarketAdapter = entityManager.getEntity<EBotMarketAdapter>(eSession->getMarketId());
      return eBotMarketAdapter ? eBotMarketAdapter->getName() : QVariant();
    }
    case Column::state:
      switch(eSession->getState())
      {
      case EBotSession::State::stopped:
        return stoppedVar;
      case EBotSession::State::active:
        return activeVar;
      case EBotSession::State::simulating:
        return simulatingVar;
      }
      break;
    }
  }
  return QVariant();
}

QVariant BotSessionModel::headerData(int section, Qt::Orientation orientation, int role) const
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
      case Column::engine:
        return tr("Engine");
      case Column::market:
        return tr("Market");
      case Column::state:
        return tr("State");
    }
  }
  return QVariant();
}

void BotSessionModel::addedEntity(Entity& entity)
{
  EBotSession* eSession = dynamic_cast<EBotSession*>(&entity);
  if(eSession)
  {
    int index = sessions.size();
    beginInsertRows(QModelIndex(), index, index);
    sessions.append(eSession);
    endInsertRows();
    return;
  }
  Q_ASSERT(false);
}

void BotSessionModel::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
{
  EBotSession* oldESession = dynamic_cast<EBotSession*>(&oldEntity);
  if(oldESession)
  {
    EBotSession* newESession = dynamic_cast<EBotSession*>(&newEntity);
    int index = sessions.indexOf(oldESession);
    sessions[index] = newESession;
    QModelIndex leftModelIndex = createIndex(index, (int)Column::first, oldESession);
    QModelIndex rightModelIndex = createIndex(index, (int)Column::last, oldESession);
    emit dataChanged(leftModelIndex, rightModelIndex);
    return;
  }
  Q_ASSERT(false);
}

void BotSessionModel::removedEntity(Entity& entity)
{
  EBotSession* eSession = dynamic_cast<EBotSession*>(&entity);
  if(eSession)
  {
    int index = sessions.indexOf(eSession);
    beginRemoveRows(QModelIndex(), index, index);
    sessions.removeAt(index);
    endRemoveRows();
    return;
  }
  Q_ASSERT(false);
}

void BotSessionModel::removedAll(quint32 type)
{
  if((EType)type == EType::botSession)
  {
    emit beginResetModel();
    sessions.clear();
    emit endResetModel();
    return;
  }
  Q_ASSERT(false);
}
