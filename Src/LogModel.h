
#pragma once

class LogModel : public QAbstractItemModel
{
public:
  LogModel();

  enum class Column
  {
      first,
      date = first,
      message,
      last = message,
  };

  enum class Type
  {
    information,
    warning,
    error,
  };

  void addMessage(Type type, const QString& message);

  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;

private:
  struct Item
  {
    Type type;
    QString date;
    QString message;
  };

  QList<Item> messages;
  QVariant errorIcon;
  QVariant warningIcon;
  QVariant informationIcon;

  virtual QModelIndex parent(const QModelIndex& child) const;
  virtual int rowCount(const QModelIndex& parent) const;
  virtual int columnCount(const QModelIndex& parent) const;
  virtual QVariant data(const QModelIndex& index, int role) const;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
};
