
#pragma once

class BotsModel : public QAbstractItemModel
{
public:
  BotsModel();
  ~BotsModel();

  enum class Column
  {
      first,
      name = first,
      state,
      last = state,
  };

  enum class State
  {
    inactive,
    active,
  };

  void addBot(const QString& name, ::Bot& botFactory);

  ::Bot* getBotFactory(const QModelIndex& index);

  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;

private:
  struct Bot
  {
    QString name;
    State state;
    ::Bot* botFactory;
  };

  QList<Bot> bots;
  QVariant activeStr;
  QVariant inactiveStr;

  virtual QModelIndex parent(const QModelIndex& child) const;
  virtual int rowCount(const QModelIndex& parent) const;
  virtual int columnCount(const QModelIndex& parent) const;
  virtual QVariant data(const QModelIndex& index, int role) const;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
};
