
#include "stdafx.h"

BotsModel::BotsModel() : state(State::offline), activeStr(tr("active")), inactiveStr(tr("inactive")) {}

BotsModel::~BotsModel()
{
  foreach(const Bot& bot, bots)
    delete bot.botFactory;
}

QString BotsModel::getStateName() const
{
  switch(state)
  {
  case State::connecting:
    return tr("connecting...");
  case State::offline:
    return tr("offline");
  case State::connected:
    return QString();
  default:
    break;
  }
  Q_ASSERT(false);
  return QString();
}

void BotsModel::setState(State state)
{
  if(this->state == state)
    return;
  this->state = state;
  emit changedState();
}

void BotsModel::addSession(quint32 id, const QString& name, const QString& engine)
{
}

void BotsModel::addBot(const QString& name, ::Bot& botFactory)
{
  int oldBotCount = bots.size();
  beginInsertRows(QModelIndex(), oldBotCount, oldBotCount);

  BotsModel::Bot bot;
  bot.name = name;
  bot.state = State::inactive;
  bot.botFactory = &botFactory;
  bots.append(bot);

  endInsertRows();
}

::Bot* BotsModel::getBotFactory(const QModelIndex& index)
{
  int row = index.row();
  if(row < 0 || row >= bots.size())
    return 0;
  return bots[row].botFactory;
}

QModelIndex BotsModel::index(int row, int column, const QModelIndex& parent) const
{
  return createIndex(row, column, 0);
}

QModelIndex BotsModel::parent(const QModelIndex& child) const
{
  return QModelIndex();
}

int BotsModel::rowCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : bots.size();
}

int BotsModel::columnCount(const QModelIndex& parent) const
{
  return (int)Column::last + 1;
}

QVariant BotsModel::data(const QModelIndex& index, int role) const
{
  if(!index.isValid())
    return QVariant();

  int row = index.row();
  if(row < 0 || row >= bots.size())
    return QVariant();
  const Bot& bot = bots[row];

  switch(role)
  {
    /*
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
    */
  case Qt::DisplayRole:
    switch((Column)index.column())
    {
    case Column::name:
      return bot.name;
    case Column::state:
      switch(bot.state)
      {
      case State::active:
        return activeStr;
      case State::inactive:
        return inactiveStr;
      default:
        break;
      }
      break;
    }
    break;
  }
  return QVariant();
}

QVariant BotsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if(orientation != Qt::Horizontal)
    return QVariant();
  switch(role)
  {
  case Qt::DisplayRole:
    switch((Column)section)
    {
      case Column::name:
        return tr("Name");
      case Column::state:
        return tr("State");
    }
    break;
  }
  return QVariant();
}
