
#pragma once

class SessionPropertyModel : public QAbstractItemModel, public Entity::Listener
{
  Q_OBJECT

public:
  enum class Column
  {
      first,
      name = first,
      value,
      last = value,
  };

public:
  SessionPropertyModel(Entity::Manager& entityManager);
  ~SessionPropertyModel();

signals:
  void editedProperty(const QModelIndex& index, const QString& value);
  void editedProperty(const QModelIndex& index, double value);

private:
  Entity::Manager& entityManager;
  QList<EBotSessionProperty*> properties;

private: // QAbstractItemModel
  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
  virtual QModelIndex parent(const QModelIndex& child) const;
  virtual int rowCount(const QModelIndex& parent) const;
  virtual int columnCount(const QModelIndex& parent) const;
  virtual QVariant data(const QModelIndex& index, int role) const;
  virtual Qt::ItemFlags flags(const QModelIndex &index) const;
  virtual bool setData(const QModelIndex & index, const QVariant & value, int role);
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private: // Entity::Listener
  virtual void addedEntity(Entity& entity);
  virtual void updatedEntitiy(Entity& oldEntity, Entity& newEntity);
  virtual void removedEntity(Entity& entity);
  virtual void removedAll(quint32 type);
};
