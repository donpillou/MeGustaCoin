
#include "stdafx.h"

LogModel::LogModel() : errorIcon(QIcon(":/Icons/cancel.png")),
warningIcon(QIcon(":/Icons/error.png")), informationIcon(QIcon(":/Icons/information.png")) {}

void LogModel::addMessage(Type type, const QString& message)
{
  int oldMessageCount = messages.size();
  beginInsertRows(QModelIndex(), oldMessageCount, oldMessageCount);

  LogModel::Item item;
  item.type = type;
  item.message = message;
  item.date = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
  messages.append(item);

  endInsertRows();
}

QModelIndex LogModel::index(int row, int column, const QModelIndex& parent) const
{
  return createIndex(row, column, 0);
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
  if(!index.isValid())
    return QVariant();

  int row = index.row();
  if(row < 0 || row >= messages.size())
    return QVariant();
  const Item& item = messages[row];

  switch(role)
  {
  case Qt::DecorationRole:
    if ((Column)index.column() == Column::message)
      switch(item.type)
      {
      case Type::error:
        return errorIcon;
      case Type::warning:
        return warningIcon;
      case Type::information:
        return informationIcon;
      }
    break;
  case Qt::DisplayRole:
    switch((Column)index.column())
    {
    case Column::date:
      return item.date;
    case Column::message:
      return item.message;
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
