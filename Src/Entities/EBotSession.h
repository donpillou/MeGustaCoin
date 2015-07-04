
#pragma once

class EBotSession : public Entity
{
public:
  static const EType eType = EType::botSession;

public:
  enum class State
  {
    stopped = meguco_user_session_stopped,
    running = meguco_user_session_running,
    starting,
    stopping,
    //simulating = meguco_user_session_simulating,
  };

  enum class Mode
  {
    live = meguco_user_session_live,
    simulation = meguco_user_session_simulation,
  };

public:
  EBotSession(quint32 tableId, const QString& name, const meguco_user_session_entity& data) : Entity(eType, tableId), name(name)
  {
    botEngineId = data.bot_engine_id;
    marketId = data.user_market_id;
    state = (State)data.state;
    mode = (Mode)data.mode;
  }

  const QString& getName() const {return name;}
  quint64 getEngineId() const {return botEngineId;}
  quint64 getMarketId() const {return marketId;}
  State getState() const {return state;}
  Mode getMode() const {return mode;}

private:
  QString name;
  quint64 botEngineId;
  quint64 marketId;
  State state;
  Mode mode;
};

