
#pragma once

class LogModel : public QAbstractItemModel, public Entity::Listener
{
public:
  enum class Column
  {
      first,
      date = first,
      message,
      last = message,
  };

public:
  LogModel(Entity::Manager& entityManager);
  ~LogModel();

private:
  class Item
  {
  public:
    ELogMessage::Type type;
    QDateTime date;
    QString message;
  };

private:
  Entity::Manager& entityManager;
  QList<Item*> messages;
  QVariant errorIcon;
  QVariant warningIcon;
  QVariant informationIcon;
  QString dateFormat;

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
  virtual void removedAll(quint32 type);
};
