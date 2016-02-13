
#include "stdafx.h"

UserSessionPropertiesModel::UserSessionPropertiesModel(Entity::Manager& entityManager) :
  entityManager(entityManager)
{
  entityManager.registerListener<EUserSessionProperty>(*this);
}

UserSessionPropertiesModel::~UserSessionPropertiesModel()
{
  entityManager.unregisterListener<EUserSessionProperty>(*this);
}

QModelIndex UserSessionPropertiesModel::index(int row, int column, const QModelIndex& parent) const
{
  if(hasIndex(row, column, parent))
    return createIndex(row, column, properties.at(row));
  return QModelIndex();
}

QModelIndex UserSessionPropertiesModel::parent(const QModelIndex& child) const
{
  return QModelIndex();
}

int UserSessionPropertiesModel::rowCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : properties.size();
}

int UserSessionPropertiesModel::columnCount(const QModelIndex& parent) const
{
  return (int)Column::last + 1;
}

QVariant UserSessionPropertiesModel::data(const QModelIndex& index, int role) const
{
  const EUserSessionProperty* eProperty = (const EUserSessionProperty*)index.internalPointer();
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
      if(eProperty->getType() == EUserSessionProperty::Type::number)
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
        QString value = eProperty->getType() == EUserSessionProperty::Type::number ? QLocale::system().toString(fabs(eProperty->getValue().toDouble()), 'g', 8) : eProperty->getValue();
        return unit.isEmpty() ? value : tr("%1 %2").arg(value, unit);
      }
    }
  }
  return QVariant();
}

Qt::ItemFlags UserSessionPropertiesModel::flags(const QModelIndex &index) const
{
  const EUserSessionProperty* eProperty = (const EUserSessionProperty*)index.internalPointer();
  if(!eProperty)
    return 0;

  Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  switch((Column)index.column())
  {
  case Column::value:
    {
      if(!(eProperty->getFlags() & EUserSessionProperty::readOnly))
        flags |= Qt::ItemIsEditable;
    }
    break;
  default:
    break;
  }
  return flags;
}

bool UserSessionPropertiesModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
  if (role != Qt::EditRole)
    return false;

  const EUserSessionProperty* eProperty = (const EUserSessionProperty*)index.internalPointer();
  if(!eProperty)
    return false;

  switch((Column)index.column())
  {
  case Column::value:
    if(!(eProperty->getFlags() & EUserSessionProperty::readOnly))
    {
      if(eProperty->getType() == EUserSessionProperty::Type::number)
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

QVariant UserSessionPropertiesModel::headerData(int section, Qt::Orientation orientation, int role) const
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

void UserSessionPropertiesModel::addedEntity(Entity& entity)
{
  switch((EType)entity.getType())
  {
  case EType::userSessionProperty:
    {
      EUserSessionProperty* eProperty = dynamic_cast<EUserSessionProperty*>(&entity);
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

void UserSessionPropertiesModel::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
{
  switch((EType)oldEntity.getType())
  {
  case EType::userSessionProperty:
    {
      EUserSessionProperty* oldEBotSessionProperty = dynamic_cast<EUserSessionProperty*>(&oldEntity);
      EUserSessionProperty* newEBotSessionProperty = dynamic_cast<EUserSessionProperty*>(&newEntity);
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

void UserSessionPropertiesModel::removedEntity(Entity& entity)
{
  switch((EType)entity.getType())
  {
  case EType::userSessionProperty:
    {
      EUserSessionProperty* eProperty = dynamic_cast<EUserSessionProperty*>(&entity);
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

void UserSessionPropertiesModel::removedAll(quint32 type)
{
  switch((EType)type)
  {
  case EType::userSessionProperty:
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
