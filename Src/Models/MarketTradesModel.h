
#pragma once

class MarketTradesModel : public QAbstractItemModel, public Entity::Listener
{
public:
  enum class Column
  {
      first,
      date = first,
      price,
      amount,
      last = amount,
  };

public:
  MarketTradesModel(Entity::Manager& channelEntityManager);
  ~MarketTradesModel();

//  void reset();
//
//  void setTrades(const QList<DataProtocol::Trade>& trades);
//  void addTrade(const DataProtocol::Trade& trade);
//
//  void clearAbove(int tradeCount);

private:
  class Trade
  {
  public:
    quint64 date;
    double price;
    double amount;

    enum class Icon
    {
      up,
      down,
      neutral,
    } icon;
  };

private:
  Entity::Manager& channelEntityManager;
  EMarketSubscription* eSubscription;
  QList<Trade*> trades;
  QVariant upIcon;
  QVariant downIcon;
  QVariant neutralIcon;

private: // QAbstractItemModel
  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
  virtual QModelIndex parent(const QModelIndex& child) const;
  virtual int rowCount(const QModelIndex& parent) const;
  virtual int columnCount(const QModelIndex& parent) const;
  virtual QVariant data(const QModelIndex& index, int role) const;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private: // Entity::Listener
  virtual void addedEntity(Entity& entity);
  virtual void updatedEntitiy(Entity& oldEntity, Entity& newEntity);
  virtual void removedEntity(Entity& entity);
  virtual void removedAll(quint32 type);
};
