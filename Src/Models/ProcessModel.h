
#pragma once

class ProcessModel : public QAbstractItemModel, public Entity::Listener
{
public:
  enum class Column
  {
      first,
      cmd = first,
      last = cmd,
  };

public:
  ProcessModel(Entity::Manager& entityManager);
  ~ProcessModel();

private:
  Entity::Manager& entityManager;
  QList<EProcess*> processes;

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

