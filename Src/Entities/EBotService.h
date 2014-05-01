
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
  EBotService() : Entity(eType, 0), state(State::offline), selectedMarketId(0) {}

  State getState() const {return state;}
  void setState(State state)
  {
    this->state = state;
    updated();
  }

  quint32 getSelectedMarketId() const {return selectedMarketId;}
  void setSelectedMarketId(quint32 id)
  {
    selectedMarketId = id;
    updated();
  }

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

private:
  State state;
  quint32 selectedMarketId;
};
