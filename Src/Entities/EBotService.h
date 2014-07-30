
#pragma once

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
  EBotService() : Entity(eType, 0), state(State::offline), selectedSessionId(0), selectedMarketId(0), loadingMarketOrders(false), loadingMarketTransactions(false) {}

  State getState() const {return state;}
  void setState(State state) {this->state = state;}

  quint32 getSelectedSessionId() const {return selectedSessionId;}
  void setSelectedSessionId(quint32 id) {selectedSessionId = id;}

  quint32 getSelectedMarketId() const {return selectedMarketId;}
  void setSelectedMarketId(quint32 id) {selectedMarketId = id;}

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
  quint32 selectedSessionId;
  quint32 selectedMarketId;
  bool loadingMarketOrders;
  bool loadingMarketTransactions;
};
