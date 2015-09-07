
#pragma once

 // todo: remove this entity

class EBotService : public Entity
{
public:
  static const EType eType = EType::botService;

public:
  enum class State
  {
    connecting,
    connected,
    offline,
  };

public:
  EBotService() : Entity(eType, 0), state(State::offline), selectedSessionId(0), selectedBrokerId(0), loadingMarketOrders(false), loadingMarketTransactions(false) {}

  State getState() const {return state;}
  void setState(State state) {this->state = state;}

  quint64 getSelectedSessionId() const {return selectedSessionId;}
  void setSelectedSessionId(quint64 id) {selectedSessionId = id;}

  quint64 getSelectedBrokerId() const {return selectedBrokerId;}
  void setSelectedBrokerId(quint64 id) {selectedBrokerId = id;}

  bool getLoadingMarketOrders() const {return loadingMarketOrders;};
  void setLoadingMarketOrders(bool loading) {loadingMarketOrders = loading;}

  bool getLoadingMarketTransactions() const {return loadingMarketTransactions;}
  void setLoadingMarketTransactions(bool loading) {loadingMarketTransactions = loading;}

  QString getStateName() const
  {
    switch(state)
    {
    case State::connecting:
      return QObject::tr("connecting...");
    case State::offline:
      return QObject::tr("offline");
    case State::connected:
      return QString();
    default:
      break;
    }
    Q_ASSERT(false);
    return QString();
  }

  QString getMarketOrdersState() const {return loadingMarketOrders ? QObject::tr("loading...") : QString();}
  QString getMarketTransitionsState() const {return loadingMarketTransactions ? QObject::tr("loading...") : QString();}

private:
  State state;
  quint64 selectedSessionId;
  quint64 selectedBrokerId;
  bool loadingMarketOrders;
  bool loadingMarketTransactions;
};
