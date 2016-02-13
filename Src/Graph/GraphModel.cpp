
#include "stdafx.h"

GraphModel::GraphModel(const QString& channeName, Entity::Manager& globalEntityManager, Entity::Manager& channelEntityManager, GraphService& graphService) :
  channelName(channeName), globalEntityManager(globalEntityManager), channelEntityManager(channelEntityManager), graphService(graphService),
  maxAge(60 * 60), enabledData(GraphRenderer::trades | GraphRenderer::expRegressionLines), addSessionMarkers(false)
{
  graphService.registerGraphModel(*this);
  channelEntityManager.registerListener<EDataTradeData>(*this);
  globalEntityManager.registerListener<EUserSessionMarker>(*this);
  globalEntityManager.registerListener<EDataService>(*this);
}

GraphModel::~GraphModel()
{
  channelEntityManager.unregisterListener<EDataTradeData>(*this);
  globalEntityManager.unregisterListener<EUserSessionMarker>(*this);
  globalEntityManager.unregisterListener<EDataService>(*this);
  graphService.unregisterGraphModel(*this);
}

void GraphModel::swapImage(QImage& image)
{
  this->image.swap(image);
}

void GraphModel::redraw()
{
  emit dataChanged();
}

void GraphModel::enable(bool enable)
{
  graphService.enable(*this, enable);
}

void GraphModel::setSize(const QSize& size)
{
  graphService.setSize(*this, size);
}

void GraphModel::setMaxAge(int maxAge)
{
  graphService.setMaxAge(*this, maxAge);
  this->maxAge = maxAge;
}

void GraphModel::setEnabledData(unsigned int data)
{
  graphService.setEnabledData(*this, data);
  this->enabledData = data;
}

void GraphModel::addedEntity(Entity& entity)
{
  EDataTradeData* eDataTradeData = dynamic_cast<EDataTradeData*>(&entity);
  if(eDataTradeData)
  {
    const QList<EDataTradeData::Trade>& data = eDataTradeData->getData(); 
    if(!data.isEmpty())
    {
      graphService.addTradeData(*this, data);
    }
    return;
  }
  EUserSessionMarker* eMarker = dynamic_cast<EUserSessionMarker*>(&entity);
  if(eMarker)
  {
    if(eMarker)
      graphService.addSessionMarker(*this, *eMarker);
    return;
  }
  Q_ASSERT(false);
}

void GraphModel::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
{
  switch((EType)newEntity.getType())
  {
  case EType::dataTradeData:
    addedEntity(newEntity);
    break;
  case EType::dataService:
    {
      bool newAddSessionMarkers = false;
      EDataService* eDataService = dynamic_cast<EDataService*>(&newEntity);
      if(eDataService->getSelectedSessionId() != 0)
      {
        EUserSession* eSession = globalEntityManager.getEntity<EUserSession>(eDataService->getSelectedSessionId());
        if(eSession && eSession->getBrokerId() != 0)
        {
          EUserBroker* eUserBroker = globalEntityManager.getEntity<EUserBroker>(eSession->getBrokerId());
          if(eUserBroker && eUserBroker->getBrokerTypeId() != 0)
          {
            EBrokerType* eBrokerType = globalEntityManager.getEntity<EBrokerType>(eUserBroker->getBrokerTypeId());
            if(eBrokerType && eBrokerType->getName() == channelName)
              newAddSessionMarkers = true;
          }
        }
      }
      if(newAddSessionMarkers != addSessionMarkers)
      {
        addSessionMarkers = newAddSessionMarkers;
        if(!addSessionMarkers)
          graphService.clearSessionMarker(*this);
        else
        {
          QList<EUserSessionMarker*> sessionMarkers;
          globalEntityManager.getAllEntities<EUserSessionMarker>(sessionMarkers);
          for(QList<EUserSessionMarker*>::ConstIterator i = sessionMarkers.begin(), end = sessionMarkers.end(); i != end; ++i)
            graphService.addSessionMarker(*this, **i);
        }
      }
    }
    break;
  default:
    break;
  }
}

void GraphModel::removedAll(quint32 type)
{
  switch((EType)type)
  {
  case EType::userSessionMarker:
    if(addSessionMarkers)
      graphService.clearSessionMarker(*this);
    break;
  default:
    break;
  }
}
