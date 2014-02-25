
#pragma once

class DataService : public QObject
{
  Q_OBJECT

public:
  DataService(DataModel& dataModel);
  ~DataService();

  void start();
  void stop();

  void subscribe(const QString& channel);
  void unsubscribe(const QString& channel);

private:
  class WorkerThread : public QThread
  {
  public:
    virtual void interrupt() = 0;
  };

  DataModel& dataModel;
  WorkerThread* thread;

  class Action
  {
  public:
    enum class Type
    {
      addTrade,
      logMessage,
      setState,
      subscribe,
      unsubscribe,
      channelInfo,
      subscribeResponse,
      unsubscribeResponse,
    };
    Type type;

    Action(Type type) : type(type) {}
    virtual ~Action() {}
  };

  class AddTradeAction : public Action
  {
  public:
    quint64 channelId;
    DataProtocol::Trade trade;
    AddTradeAction(quint64 channelId, const DataProtocol::Trade& trade) : Action(Type::addTrade), channelId(channelId), trade(trade) {}
  };

  class LogMessageAction : public Action
  {
  public:
    LogModel::Type type;
    QString message;
    LogMessageAction(LogModel::Type type, const QString& message) : Action(Type::logMessage), type(type), message(message) {}
  };

  class SetStateAction : public Action
  {
  public:
    PublicDataModel::State state;
    SetStateAction(PublicDataModel::State state) : Action(Type::setState), state(state) {}
  };

  class ChannelInfoAction : public Action
  {
  public:
    QString channel;
    ChannelInfoAction(const QString& channel) : Action(Type::channelInfo), channel(channel) {}
  };

  class SubscriptionAction : public Action
  {
  public:
    QString channel;
    quint64 lastReceivedTradeId;
    SubscriptionAction(Type type, const QString& channel, quint64 lastReceivedTradeId) : Action(type), channel(channel), lastReceivedTradeId(lastReceivedTradeId) {}
  };

  class SubscribeResponseAction : public Action
  {
  public:
    QString channel;
    quint64 channelId;
    SubscribeResponseAction(Type type, const QString& channel, quint64 channelId) : Action(type), channel(channel), channelId(channelId) {}
  };

  JobQueue<Action*> actionQueue;
  JobQueue<Action*> subscriptionQueue;
  QHash<quint64, PublicDataModel*> activeSubscriptions;
  bool isConnected;
  QSet<QString> subscriptions;

private slots:
  void executeActions();
};
