
#include "stdafx.h"

LogModel::LogModel(Entity::Manager& entityManager) :
  entityManager(entityManager),
  errorIcon(QIcon(":/Icons/cancel.png")), warningIcon(QIcon(":/Icons/error.png")), informationIcon(QIcon(":/Icons/information.png")),
  dateFormat(QLocale::system().dateTimeFormat(QLocale::ShortFormat))
{
  entityManager.registerListener<ELogMessage>(*this);
}

LogModel::~LogModel()
{
  entityManager.unregisterListener<ELogMessage>(*this);

  qDeleteAll(messages);
}

QModelIndex LogModel::index(int row, int column, const QModelIndex& parent) const
{
  return createIndex(row, column, messages.at(row));
}

QModelIndex LogModel::parent(const QModelIndex& child) const
{
  return QModelIndex();
}

int LogModel::rowCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : messages.size();
}

int LogModel::columnCount(const QModelIndex& parent) const
{
  return (int)Column::last + 1;
}

QVariant LogModel::data(const QModelIndex& index, int role) const
{
  const Item* item = (const Item*)index.internalPointer();
  if(!item)
    return QVariant();

  switch(role)
  {
  case Qt::DecorationRole:
    if ((Column)index.column() == Column::message)
      switch(item->type)
      {
      case ELogMessage::Type::error:
        return errorIcon;
      case ELogMessage::Type::warning:
        return warningIcon;
      case ELogMessage::Type::information:
        return informationIcon;
      }
    break;
  case Qt::DisplayRole:
    switch((Column)index.column())
    {
    case Column::date:
      return item->date.toString(dateFormat);
    case Column::message:
      return item->message;
    }
    break;
  }
  return QVariant();
}

QVariant LogModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if(orientation != Qt::Horizontal)
    return QVariant();
  switch(role)
  {
  case Qt::DisplayRole:
    switch((Column)section)
    {
      case Column::date:
        return tr("Date");
      case Column::message:
        return tr("Message");
    }
    break;
  }
  return QVariant();
}

void LogModel::addedEntity(Entity& entity)
{
  ELogMessage* eLogMessage = dynamic_cast<ELogMessage*>(&entity);
  if(eLogMessage)
  {
    int index = messages.size();
    Item* item = new Item;
    item->type = eLogMessage->getType();
    item->date = eLogMessage->getDate();
    item->message = eLogMessage->getMessage();
    beginInsertRows(QModelIndex(), index, index);
    messages.append(item);
    endInsertRows();
    return;
  }
  Q_ASSERT(false);
}

void LogModel::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
{
  addedEntity(newEntity);
}

void LogModel::removedAll(quint32 type)
{
  if((EType)type == EType::logMessage)
  {
    emit beginResetModel();
    qDeleteAll(messages);
    messages.clear();
    emit endResetModel();
    return;
  }
  Q_ASSERT(false);
}