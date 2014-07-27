
#include "stdafx.h"

GraphModel::GraphModel(const QString& channeName, Entity::Manager& channelEntityManager, GraphService& graphService) :
  channelName(channeName), channelEntityManager(channelEntityManager), graphService(graphService),
  maxAge(60 * 60), enabledData(GraphRenderer::trades | GraphRenderer::expRegressionLines)
{
  graphService.registerGraphModel(*this);
  channelEntityManager.registerListener<EDataTradeData>(*this);
}

GraphModel::~GraphModel()
{
  channelEntityManager.unregisterListener<EDataTradeData>(*this);
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
  Q_ASSERT(false);
}

void GraphModel::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
{
  addedEntity(newEntity);
}
