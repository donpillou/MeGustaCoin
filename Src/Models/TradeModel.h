
#pragma once

class TradeModel : public QAbstractItemModel
{
  Q_OBJECT

public:
  TradeModel(DataModel& dataModel);
  ~TradeModel();

  class Trade : public Market::Trade
  {
  public:
    enum class Icon
    {
      up,
      down,
      neutral,
    } icon;

    //Trade() : amount(0.), price(0.), icon(Icon::neutral) {}
    Trade(const Market::Trade& trade) : Market::Trade(trade), icon(Icon::neutral) {}
  };

  enum class Column
  {
      first,
      date = first,
      price,
      amount,
      last = amount,
  };

  void reset();

  void addData(const QList<Market::Trade>& trades);

  void clearAbove(int tradeCount);

  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;

private:
  DataModel& dataModel;
  GraphModel& graphModel;
  QList<Trade*> trades;
  QHash<QString, void*> ids;
  QVariant upIcon;
  QVariant downIcon;
  QVariant neutralIcon;

  virtual QModelIndex parent(const QModelIndex& child) const;
  virtual int rowCount(const QModelIndex& parent) const;
  virtual int columnCount(const QModelIndex& parent) const;
  virtual QVariant data(const QModelIndex& index, int role) const;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private slots:
  void updateHeader();
};
