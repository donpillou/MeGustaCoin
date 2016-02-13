
#include "stdafx.h"

UserSessionsModel::UserSessionsModel(Entity::Manager& entityManager) : entityManager(entityManager),
  stoppedVar(tr("stopped")), startingVar(tr("starting")), runningVar(tr("running")), simulatingVar(tr("simulating")), stoppingVar(tr("stopping"))
{
  entityManager.registerListener<EUserSession>(*this);
}

UserSessionsModel::~UserSessionsModel()
{
  entityManager.unregisterListener<EUserSession>(*this);
}

QModelIndex UserSessionsModel::index(int row, int column, const QModelIndex& parent) const
{
  if(hasIndex(row, column, parent))
    return createIndex(row, column, sessions.at(row));
  return QModelIndex();
}

QModelIndex UserSessionsModel::parent(const QModelIndex& child) const
{
  return QModelIndex();
}

int UserSessionsModel::rowCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : sessions.size();
}

int UserSessionsModel::columnCount(const QModelIndex& parent) const
{
  return (int)Column::last + 1;
}

QVariant UserSessionsModel::data(const QModelIndex& index, int role) const
{
  const EUserSession* eSession = (const EUserSession*)index.internalPointer();
  if(!eSession)
    return QVariant();

  switch(role)
  {
  case Qt::DisplayRole:
    switch((Column)index.column())
    {
    case Column::name:
      return eSession->getName();
    case Column::type:
    {
      EBotType* eBotType = entityManager.getEntity<EBotType>(eSession->getBotTypeId());
      return eBotType ? eBotType->getName() : QVariant();
    }
    case Column::market:
    {
      EUserBroker* eUserBroker = entityManager.getEntity<EUserBroker>(eSession->getBrokerId());
      EBrokerType* eBrokerType = eUserBroker ? entityManager.getEntity<EBrokerType>(eUserBroker->getBrokerTypeId()) : 0;
      return eBrokerType ? eBrokerType->getName() : QVariant();
    }
    case Column::state:
      switch(eSession->getState())
      {
      case EUserSession::State::stopped:
        return stoppedVar;
      case EUserSession::State::starting:
        return startingVar;
      case EUserSession::State::stopping:
        return stoppingVar;
      case EUserSession::State::running:
        return eSession->getMode() == EUserSession::Mode::simulation ? simulatingVar : runningVar;
      }
      break;
    }
  }
  return QVariant();
}

QVariant UserSessionsModel::headerData(int section, Qt::Orientation orientation, int role) const
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
      case Column::type:
        return tr("Type");
      case Column::market:
        return tr("Market");
      case Column::state:
        return tr("State");
    }
  }
  return QVariant();
}

void UserSessionsModel::addedEntity(Entity& entity)
{
  EUserSession* eSession = dynamic_cast<EUserSession*>(&entity);
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

void UserSessionsModel::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
{
  EUserSession* oldESession = dynamic_cast<EUserSession*>(&oldEntity);
  if(oldESession)
  {
    EUserSession* newESession = dynamic_cast<EUserSession*>(&newEntity);
    int index = sessions.indexOf(oldESession);
    sessions[index] = newESession;
    QModelIndex leftModelIndex = createIndex(index, (int)Column::first, oldESession);
    QModelIndex rightModelIndex = createIndex(index, (int)Column::last, oldESession);
    emit dataChanged(leftModelIndex, rightModelIndex);
    return;
  }
  Q_ASSERT(false);
}

void UserSessionsModel::removedEntity(Entity& entity)
{
  EUserSession* eSession = dynamic_cast<EUserSession*>(&entity);
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

void UserSessionsModel::removedAll(quint32 type)
{
  if((EType)type == EType::userSession)
  {
    emit beginResetModel();
    sessions.clear();
    emit endResetModel();
    return;
  }
  Q_ASSERT(false);
}
