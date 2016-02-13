
#pragma once

class EConnection : public Entity
{
public:
  static const EType eType = EType::connection;

public:
  enum class State
  {
    connecting,
    connected,
    offline,
  };

public:
  EConnection() : Entity(eType, 0), state(State::offline), selectedSessionId(0), selectedBrokerId(0), loadingMarketOrders(false), loadingMarketTransactions(false) {}

  State getState() const {return state;}
  void setState(State state) {this->state = state;}

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

  quint32 getSelectedSessionId() const {return selectedSessionId;}
  void setSelectedSessionId(quint32 id) {selectedSessionId = id;}

  quint32 getSelectedBrokerId() const {return selectedBrokerId;}
  void setSelectedBrokerId(quint32 id) {selectedBrokerId = id;}

  bool getLoadingBrokerOrders() const {return loadingMarketOrders;};
  void setLoadingBrokerOrders(bool loading) {loadingMarketOrders = loading;}

  bool getLoadingBrokerTransactions() const {return loadingMarketTransactions;}
  void setLoadingBrokerTransactions(bool loading) {loadingMarketTransactions = loading;}

private:
  State state;
  quint32 selectedSessionId;
  quint32 selectedBrokerId;
  bool loadingMarketOrders;
  bool loadingMarketTransactions;
};
