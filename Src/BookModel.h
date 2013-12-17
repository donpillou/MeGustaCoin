
#pragma once

class BookModel
{
public:
  BookModel();

  class Item
  {
  public:
    double amount;
    double price;

    Item() : amount(0.), price(0.) {}
  };

  class ItemModel : public QAbstractItemModel
  {
  public:

    ItemModel();
    ~ItemModel();

    void reset();

    void setData(const QList<Item>& items);

    virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;

  private:
    QList<Item*> items;
    Market* market;

    virtual QModelIndex parent(const QModelIndex& child) const;
    virtual int rowCount(const QModelIndex& parent) const;
    virtual int columnCount(const QModelIndex& parent) const;
    virtual QVariant data(const QModelIndex& index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    friend class BookModel;
  };

  enum class Column
  {
      first,
      price = first,
      amount,
      last = amount,
  };

  void setMarket(Market* market);

  void reset();

  ItemModel askModel;
  ItemModel bidModel;
};
