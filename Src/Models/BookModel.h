
#pragma once

class BookModel : public QObject
{
  Q_OBJECT

public:
  BookModel(PublicDataModel& publicDataModel);

  class ItemModel : public QAbstractItemModel
  {
  public:
    ItemModel(PublicDataModel& publicDataModel);
    ~ItemModel();

    void reset();

    void setData(const QList<MarketStream::OrderBookEntry>& items);

    void updateHeader();

    virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;

  private:
    PublicDataModel& publicDataModel;
    QList<MarketStream::OrderBookEntry*> items;

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

  void setData(quint64 time, const QList<MarketStream::OrderBookEntry>& askItems, const QList<MarketStream::OrderBookEntry>& bidItems);
  quint64 getTime() const {return time;}

  void reset();

  ItemModel askModel;
  ItemModel bidModel;

private:
  PublicDataModel& publicDataModel;
  quint64 time;

private slots:
  void updateHeader();
};
