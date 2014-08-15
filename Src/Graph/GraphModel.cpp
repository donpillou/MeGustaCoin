
#include "stdafx.h"

GraphModel::GraphModel(const QString& channeName, Entity::Manager& globalEntityManager, Entity::Manager& channelEntityManager, GraphService& graphService) :
  channelName(channeName), globalEntityManager(globalEntityManager), channelEntityManager(channelEntityManager), graphService(graphService),
  maxAge(60 * 60), enabledData(GraphRenderer::trades | GraphRenderer::expRegressionLines), addSessionMarkers(false)
{
  graphService.registerGraphModel(*this);
  channelEntityManager.registerListener<EDataTradeData>(*this);
  globalEntityManager.registerListener<EBotSessionMarker>(*this);
  globalEntityManager.registerListener<EBotService>(*this);
}

GraphModel::~GraphModel()
{
  channelEntityManager.unregisterListener<EDataTradeData>(*this);
  globalEntityManager.unregisterListener<EBotSessionMarker>(*this);
  globalEntityManager.unregisterListener<EBotService>(*this);
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
    const QList<DataProtocol::Trade>& data = eDataTradeData->getData(); 
    if(!data.isEmpty())
    {
      graphService.addTradeData(*this, data);
    }
    return;
  }
  EBotSessionMarker* eBotSessionMarker = dynamic_cast<EBotSessionMarker*>(&entity);
  if(eBotSessionMarker)
  {
    if(addSessionMarkers)
      graphService.addSessionMarker(*this, *eBotSessionMarker);
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
  case EType::botService:
    {
      bool newAddSessionMarkers = false;
      EBotService* eBotService = dynamic_cast<EBotService*>(&newEntity);
      if(eBotService->getSelectedSessionId() != 0)
      {
        EBotSession* eBotSession = globalEntityManager.getEntity<EBotSession>(eBotService->getSelectedSessionId());
        if(eBotSession && eBotSession->getMarketId() != 0)
        {
          EBotMarket* eBotMarket = globalEntityManager.getEntity<EBotMarket>(eBotSession->getMarketId());
          if(eBotMarket && eBotMarket->getMarketAdapterId() != 0)
          {
            EBotMarketAdapter* eBotMarketAdapter = globalEntityManager.getEntity<EBotMarketAdapter>(eBotMarket->getMarketAdapterId());
            if(eBotMarketAdapter && eBotMarketAdapter->getName() == channelName)
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
          QList<EBotSessionMarker*> sessionMarkers;
          globalEntityManager.getAllEntities<EBotSessionMarker>(sessionMarkers);
          for(QList<EBotSessionMarker*>::ConstIterator i = sessionMarkers.begin(), end = sessionMarkers.end(); i != end; ++i)
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
  case EType::botSessionMarker:
    if(addSessionMarkers)
      graphService.clearSessionMarker(*this);
    break;
  default:
    break;
  }
}
