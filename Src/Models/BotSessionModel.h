
#pragma once

class BotSessionModel : public QAbstractItemModel, public Entity::Listener
{
public:
  enum class Column
  {
      first,
      name = first,
      engine,
      market,
      state,
      balanceBase,
      balanceComm,
      last = balanceComm,
  };

public:
  BotSessionModel(Entity::Manager& entityManager);
  ~BotSessionModel();

private:
  Entity::Manager& entityManager;
  QList<EBotSession*> sessions;

  QVariant stoppedVar;
  QVariant startingVar;
  QVariant runningVar;
  QVariant simulatingVar;

private: // QAbstractItemModel
  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
  virtual QModelIndex parent(const QModelIndex& child) const;
  virtual int rowCount(const QModelIndex& parent) const;
  virtual int columnCount(const QModelIndex& parent) const;
  virtual QVariant data(const QModelIndex& index, int role) const;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private: // Entity::Listener
  virtual void addedEntity(Entity& entity);
  virtual void updatedEntitiy(Entity& oldEntity, Entity& newEntity);
  virtual void removedEntity(Entity& entity);
  virtual void removedAll(quint32 type);
};
