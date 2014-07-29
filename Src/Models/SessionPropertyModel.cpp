
#include "stdafx.h"

SessionPropertyModel::SessionPropertyModel(Entity::Manager& entityManager) :
  entityManager(entityManager)
{
  entityManager.registerListener<EBotSessionProperty>(*this);
}

SessionPropertyModel::~SessionPropertyModel()
{
  entityManager.unregisterListener<EBotSessionProperty>(*this);
}

QModelIndex SessionPropertyModel::index(int row, int column, const QModelIndex& parent) const
{
  if(hasIndex(row, column, parent))
    return createIndex(row, column, properties.at(row));
  return QModelIndex();
}

QModelIndex SessionPropertyModel::parent(const QModelIndex& child) const
{
  return QModelIndex();
}

int SessionPropertyModel::rowCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : properties.size();
}

int SessionPropertyModel::columnCount(const QModelIndex& parent) const
{
  return (int)Column::last + 1;
}

QVariant SessionPropertyModel::data(const QModelIndex& index, int role) const
{
  const EBotSessionProperty* eProperty = (const EBotSessionProperty*)index.internalPointer();
  if(!eProperty)
    return QVariant();

  switch(role)
  {
  case Qt::TextAlignmentRole:
    switch((Column)index.column())
    {
    case Column::value:
      return (int)Qt::AlignRight | (int)Qt::AlignVCenter;
    default:
      return (int)Qt::AlignLeft | (int)Qt::AlignVCenter;
    }
  case Qt::EditRole:
    switch((Column)index.column())
    {
    case Column::value:
      if(eProperty->getType() == EBotSessionProperty::Type::number)
        return eProperty->getValue().toDouble();
      else
        return eProperty->getValue();
    default:
      break;
    }
    break;
  case Qt::DisplayRole:
    switch((Column)index.column())
    {
    case Column::name:
      return eProperty->getName();
    case Column::value:
      {
        const QString& unit = eProperty->getUnit();
        QString value = eProperty->getType() == EBotSessionProperty::Type::number ? QLocale::system().toString(fabs(eProperty->getValue().toDouble()), 'g', 8) : eProperty->getValue();
        return unit.isEmpty() ? value : tr("%1 %2").arg(value, unit);
      }
    }
  }
  return QVariant();
}

Qt::ItemFlags SessionPropertyModel::flags(const QModelIndex &index) const
{
  const EBotSessionProperty* eProperty = (const EBotSessionProperty*)index.internalPointer();
  if(!eProperty)
    return 0;

  Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  switch((Column)index.column())
  {
  case Column::value:
    {
      if(!(eProperty->getFlags() & EBotSessionProperty::readOnly))
        flags |= Qt::ItemIsEditable;
    }
    break;
  default:
    break;
  }
  return flags;
}

bool SessionPropertyModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
  if (role != Qt::EditRole)
    return false;

  const EBotSessionProperty* eProperty = (const EBotSessionProperty*)index.internalPointer();
  if(!eProperty)
    return false;

  switch((Column)index.column())
  {
  case Column::value:
    if(!(eProperty->getFlags() & EBotSessionProperty::readOnly))
    {
      if(eProperty->getType() == EBotSessionProperty::Type::number)
      {
        double newValue = value.toDouble();
        if(newValue != eProperty->getValue().toDouble())
          editedProperty(index, newValue);
      }
      else
      {
        QString newValue = value.toString();
        if(newValue != eProperty->getValue())
          editedProperty(index, newValue);
      }
      return true;
    }
    break;
  default:
    break;
  }
  return false;
}

QVariant SessionPropertyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if(orientation != Qt::Horizontal)
    return QVariant();
  switch(role)
  {
  case Qt::TextAlignmentRole:
    switch((Column)section)
    {
    case Column::value:
      return Qt::AlignRight;
    default:
      return Qt::AlignLeft;
    }
  case Qt::DisplayRole:
    switch((Column)section)
    {
    case Column::name:
      return tr("Name");
    case Column::value:
      return tr("Value");
    }
  }
  return QVariant();
}

void SessionPropertyModel::addedEntity(Entity& entity)
{
  switch((EType)entity.getType())
  {
  case EType::botSessionProperty:
    {
      EBotSessionProperty* eProperty = dynamic_cast<EBotSessionProperty*>(&entity);
      int index = properties.size();
      beginInsertRows(QModelIndex(), index, index);
      properties.append(eProperty);
      endInsertRows();
      break;
    }
  default:
    Q_ASSERT(false);
    break;
  }
}

void SessionPropertyModel::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
{
  switch((EType)oldEntity.getType())
  {
  case EType::botSessionProperty:
    {
      EBotSessionProperty* oldEBotSessionProperty = dynamic_cast<EBotSessionProperty*>(&oldEntity);
      EBotSessionProperty* newEBotSessionProperty = dynamic_cast<EBotSessionProperty*>(&newEntity);
      int index = properties.indexOf(oldEBotSessionProperty);
      properties[index] = newEBotSessionProperty; 
      QModelIndex leftModelIndex = createIndex(index, (int)Column::first, newEBotSessionProperty);
      QModelIndex rightModelIndex = createIndex(index, (int)Column::last, newEBotSessionProperty);
      emit dataChanged(leftModelIndex, rightModelIndex);
      break;
    }
  default:
    Q_ASSERT(false);
    break;
  }
}

void SessionPropertyModel::removedEntity(Entity& entity)
{
  switch((EType)entity.getType())
  {
  case EType::botSessionProperty:
    {
      EBotSessionProperty* eProperty = dynamic_cast<EBotSessionProperty*>(&entity);
      int index = properties.indexOf(eProperty);
      beginRemoveRows(QModelIndex(), index, index);
      properties.removeAt(index);
      endRemoveRows();
      break;
    }
  default:
    Q_ASSERT(false);
    break;
  }
}

void SessionPropertyModel::removedAll(quint32 type)
{
  switch((EType)type)
  {
  case EType::botSessionProperty:
    if(!properties.isEmpty())
    {
      emit beginResetModel();
      properties.clear();
      emit endResetModel();
    }
    break;
  default:
    Q_ASSERT(false);
    break;
  }
}
