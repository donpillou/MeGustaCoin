
#pragma once

class MarketStreamService : public QObject
{
  Q_OBJECT

public:
  MarketStreamService(QObject* parent, DataModel& dataModel, PublicDataModel& publicDataModel, const QString& marketName);
  ~MarketStreamService();

  void subscribe();
  void unsubscribe();

private:
  DataModel& dataModel;
  PublicDataModel& publicDataModel;
  QString marketName;
  MarketStream* marketStream;
  QThread* thread;

  class Action
  {
  public:
    enum class Type
    {
      addTrade,
      error
    };
    Type type;

    Action(Type type) : type(type) {}
    virtual ~Action() {}
  };

  class AddTradeAction : public Action
  {
  public:
    MarketStream::Trade trade;
    AddTradeAction(const MarketStream::Trade& trade) : Action(Type::addTrade), trade(trade) {}
  };

  class ErrorAction : public Action
  {
  public:
    QString message;
    ErrorAction(const QString& message) : Action(Type::error), message(message) {}
  };

  JobQueue<Action*> actionQueue;

private slots:
  void executeActions();
};
