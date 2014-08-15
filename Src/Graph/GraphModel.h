
#pragma once

class GraphService;

class GraphModel : public QObject, public Entity::Listener
{
  Q_OBJECT

public:
  GraphModel(const QString& channelName, Entity::Manager& globalEntityManager, Entity::Manager& channelEntityManager, GraphService& graphService);
  ~GraphModel();

  const QString& getChannelName() const {return channelName;}
  const QImage& getImage() const {return image;}

  void swapImage(QImage& image);
  void redraw();

  void enable(bool enable);
  void setSize(const QSize& size);
  void setMaxAge(int maxAge);
  int getMaxAge() const {return maxAge;}
  void setEnabledData(unsigned int data);
  unsigned int getEnabledData() const {return enabledData;}

signals:
  void dataChanged();

private:
  QString channelName;
  Entity::Manager& globalEntityManager;
  Entity::Manager& channelEntityManager;
  GraphService& graphService;
  QImage image;

  int maxAge;
  unsigned int enabledData;

  bool addSessionMarkers;

private: // Entity::Listener
  virtual void addedEntity(Entity& entity);
  virtual void updatedEntitiy(Entity& oldEntity, Entity& newEntity);
  virtual void removedAll(quint32 type);
};
