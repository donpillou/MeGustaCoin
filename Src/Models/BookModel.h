
#pragma once

class BookModel : public QObject
{
  Q_OBJECT

public:
  BookModel(DataModel& dataModel);

  class ItemModel : public QAbstractItemModel
  {
  public:
    ItemModel(DataModel& dataModel);
    ~ItemModel();

    void reset();

    void setData(const QList<Market::OrderBookEntry>& items);

    void updateHeader();

    virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;

  private:
    DataModel& dataModel;
    QList<Market::OrderBookEntry*> items;

    virtual QModelIndex parent(const QModelIndex& child) const;
    virtual int rowCount(const QModelIndex& parent) const;
    virtual int columnCount(const QModelIndex& parent) const;
    virtual QVariant data(const QModelIndex& index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  };

  enum class Column
  {
      first,
      price = first,
      amount,
      last = amount,
  };

  void setData(quint64 time, const QList<Market::OrderBookEntry>& askItems, const QList<Market::OrderBookEntry>& bidItems);

  void reset();

  ItemModel askModel;
  ItemModel bidModel;

private:
  DataModel& dataModel;
  GraphModel& graphModel;
  quint64 time;

private slots:
  void updateHeader();
};
