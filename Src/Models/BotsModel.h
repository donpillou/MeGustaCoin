
#pragma once

class BotsModel : public QAbstractItemModel
{
  Q_OBJECT

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
    connecting,
    connected,
    offline,

    inactive, // que?
    active,
  };

  void addBot(const QString& name, ::Bot& botFactory);

  ::Bot* getBotFactory(const QModelIndex& index);

  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;

  State getState() const {return state;}
  QString getStateName() const;
  void setState(State state);

  void addEngine(const QString& engine) {engines.append(engine);}
  const QList<QString>& getEngines() const {return engines;}

  void addSession(quint32 id, const QString& name, const QString& engine);

signals:
  void changedState();

private:
  struct Bot
  {
    QString name;
    State state;
    ::Bot* botFactory;
  };

  State state;
  QList<QString> engines;
  QList<Bot> bots;
  QVariant activeStr;
  QVariant inactiveStr;

  virtual QModelIndex parent(const QModelIndex& child) const;
  virtual int rowCount(const QModelIndex& parent) const;
  virtual int columnCount(const QModelIndex& parent) const;
  virtual QVariant data(const QModelIndex& index, int role) const;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
};
