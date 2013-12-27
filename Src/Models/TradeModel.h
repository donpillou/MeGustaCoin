
#pragma once

class TradeModel : public QAbstractItemModel
{
public:
  TradeModel(GraphModel& graphModel);
  ~TradeModel();

  class Trade
  {
  public:
    QString id;
    quint64 date;
    double amount;
    double price;
    enum class Icon
    {
      up,
      down,
      neutral,
    } icon;

    Trade() : amount(0.), price(0.), icon(Icon::neutral) {}
  };

  enum class Column
  {
      first,
      date = first,
      price,
      amount,
      last = amount,
  };

  void setMarket(Market* market);

  void reset();

  void addData(const QList<Trade>& trades);

  void clearAbove(int tradeCount);

  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;

private:
  GraphModel& graphModel;
  QList<Trade*> trades;
  QHash<QString, void*> ids;
  Market* market;
  QVariant upIcon;
  QVariant downIcon;
  QVariant neutralIcon;

  virtual QModelIndex parent(const QModelIndex& child) const;
  virtual int rowCount(const QModelIndex& parent) const;
  virtual int columnCount(const QModelIndex& parent) const;
  virtual QVariant data(const QModelIndex& index, int role) const;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
};
