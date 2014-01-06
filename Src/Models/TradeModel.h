
#pragma once

class PublicDataModel;

class TradeModel : public QAbstractItemModel
{
  Q_OBJECT

public:
  TradeModel(PublicDataModel& publicDataModel);
  ~TradeModel();

  class Trade : public MarketStream::Trade
  {
  public:
    enum class Icon
    {
      up,
      down,
      neutral,
    } icon;

    Trade(const MarketStream::Trade& trade) : MarketStream::Trade(trade), icon(Icon::neutral) {}
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

  void addTrade(const MarketStream::Trade& trade);

  void clearAbove(int tradeCount);

  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;

private:
  PublicDataModel& publicDataModel;
  QList<Trade*> trades;
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
