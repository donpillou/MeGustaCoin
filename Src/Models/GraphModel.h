
#pragma once

class GraphModel : public QObject, public Entity::Listener
{
  Q_OBJECT

public:
  class TradeSample
  {
  public:
    quint64 time;
    double min;
    double max;
    double first;
    double last;
    double amount;

    TradeSample() : amount(0) {}
  };

public:
  GraphModel(Entity::Manager& channelEntityManager);
  ~GraphModel();

  const QList<TradeSample>& getTradeSamples() const {return tradeSamples;}
  const TradeHandler::Values* getValues() const {return values;}

signals:
  void dataAdded();

private:
  Entity::Manager& channelEntityManager;
  TradeHandler tradeHander;

  QList<TradeSample> tradeSamples;
  TradeHandler::Values* values;

private:
  void addTrade(const DataProtocol::Trade& trade, quint64 tradeAge);

private: // Entity::Listener
  virtual void addedEntity(Entity& entity);
  virtual void updatedEntitiy(Entity& oldEntity, Entity& newEntity);
};
