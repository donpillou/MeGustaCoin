
#include "stdafx.h"

BotSessionModel::BotSessionModel(Entity::Manager& entityManager) : entityManager(entityManager),
  stoppedVar(tr("stopped")), startingVar(tr("starting")), runningVar(tr("running")), simulatingVar(tr("simulating"))
{
  entityManager.registerListener<EBotSession>(*this);
  entityManager.registerListener<EBotSessionBalance>(*this);
  entityManager.registerListener<EBotService>(*this);
}

BotSessionModel::~BotSessionModel()
{
  entityManager.unregisterListener<EBotSession>(*this);
  entityManager.unregisterListener<EBotSessionBalance>(*this);
  entityManager.unregisterListener<EBotService>(*this);
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
  case Qt::TextAlignmentRole:
    switch((Column)index.column())
    {
    case Column::balanceBase:
    case Column::balanceComm:
      return (int)Qt::AlignRight | (int)Qt::AlignVCenter;
    default:
      return (int)Qt::AlignLeft | (int)Qt::AlignVCenter;
    }
    break;
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
      EBotMarket* eBotMarket = entityManager.getEntity<EBotMarket>(eSession->getMarketId());
      EBotMarketAdapter* eBotMarketAdapter = entityManager.getEntity<EBotMarketAdapter>(eBotMarket->getMarketAdapterId());
      return eBotMarketAdapter ? eBotMarketAdapter->getName() : QVariant();
    }
    case Column::state:
      switch(eSession->getState())
      {
      case EBotSession::State::stopped:
        return stoppedVar;
      case EBotSession::State::starting:
        return startingVar;
      case EBotSession::State::running:
        return runningVar;
      case EBotSession::State::simulating:
        return simulatingVar;
      }
      break;
    case Column::balanceBase:
      {
        EBotService* eBotService = entityManager.getEntity<EBotService>(0);
        EBotSessionBalance* eBotSessionBalance = 0;
        EBotMarketAdapter* eBotMarketAdapter = 0;
        if(eBotService->getSelectedSessionId() == eSession->getId())
        {
          eBotSessionBalance = entityManager.getEntity<EBotSessionBalance>(0);
          if(eBotSessionBalance)
          {
            EBotMarket* eBotMarket = entityManager.getEntity<EBotMarket>(eSession->getMarketId());
            if(eBotMarket)
              eBotMarketAdapter = entityManager.getEntity<EBotMarketAdapter>(eBotMarket->getMarketAdapterId());
          }
        }
        return eBotMarketAdapter ? eBotMarketAdapter->formatPrice(eBotSessionBalance->getAvailableUsd()) : QString();
      }
    case Column::balanceComm:
      {
        EBotService* eBotService = entityManager.getEntity<EBotService>(0);
        EBotSessionBalance* eBotSessionBalance = 0;
        EBotMarketAdapter* eBotMarketAdapter = 0;
        if(eBotService->getSelectedSessionId() == eSession->getId())
        {
          eBotSessionBalance = entityManager.getEntity<EBotSessionBalance>(0);
          if(eBotSessionBalance)
          {
            EBotMarket* eBotMarket = entityManager.getEntity<EBotMarket>(eSession->getMarketId());
            if(eBotMarket)
              eBotMarketAdapter = entityManager.getEntity<EBotMarketAdapter>(eBotMarket->getMarketAdapterId());
          }
        }
        return eBotMarketAdapter ? eBotMarketAdapter->formatPrice(eBotSessionBalance->getAvailableBtc()) : QString();
      }
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
      case Column::balanceBase:
        {
          EBotMarketAdapter* eBotMarketAdapter = 0;
          EBotService* eBotService = entityManager.getEntity<EBotService>(0);
          EBotSession* eBotSession = entityManager.getEntity<EBotSession>(eBotService->getSelectedSessionId());
          if(eBotSession)
          {
            EBotMarket* eBotMarket = entityManager.getEntity<EBotMarket>(eBotSession->getMarketId());
            if(eBotMarket)
              eBotMarketAdapter = entityManager.getEntity<EBotMarketAdapter>(eBotMarket->getMarketAdapterId());
          }
          return tr("Balance %1").arg(eBotMarketAdapter ? eBotMarketAdapter->getBaseCurrency() : QString());
        }
      case Column::balanceComm:
        {
          EBotMarketAdapter* eBotMarketAdapter = 0;
          EBotService* eBotService = entityManager.getEntity<EBotService>(0);
          EBotSession* eBotSession = entityManager.getEntity<EBotSession>(eBotService->getSelectedSessionId());
          if(eBotSession)
          {
            EBotMarket* eBotMarket = entityManager.getEntity<EBotMarket>(eBotSession->getMarketId());
            if(eBotMarket)
              eBotMarketAdapter = entityManager.getEntity<EBotMarketAdapter>(eBotMarket->getMarketAdapterId());
          }
          return tr("Balance %1").arg(eBotMarketAdapter ? eBotMarketAdapter->getCommCurrency() : QString());
        }
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
  EBotSessionBalance* eSessionBalance = dynamic_cast<EBotSessionBalance*>(&entity);
  if(eSessionBalance)
  {
    if(!sessions.isEmpty())
      dataChanged(index(0, (int)Column::balanceBase), index(sessions.size() - 1, (int)Column::balanceComm));
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
  EBotSessionBalance* eSessionBalance = dynamic_cast<EBotSessionBalance*>(&newEntity);
  if(eSessionBalance)
  {
    if(!sessions.isEmpty())
      dataChanged(index(0, (int)Column::balanceBase), index(sessions.size() - 1, (int)Column::balanceComm));
    return;
  }
  EBotService* eBotService = dynamic_cast<EBotService*>(&newEntity);
  if(eBotService)
  {
    headerDataChanged(Qt::Horizontal, (int)Column::balanceBase, (int)Column::balanceComm);
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
  EBotSessionBalance* eSessionBalance = dynamic_cast<EBotSessionBalance*>(&entity);
  if(eSessionBalance)
  {
    if(!sessions.isEmpty())
      dataChanged(index(0, (int)Column::balanceBase), index(sessions.size() - 1, (int)Column::balanceComm));
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
  else if((EType)type == EType::botSessionBalance)
  {
    if(!sessions.isEmpty())
      dataChanged(index(0, (int)Column::balanceBase), index(sessions.size() - 1, (int)Column::balanceComm));
    return;
  }
  Q_ASSERT(false);
}
