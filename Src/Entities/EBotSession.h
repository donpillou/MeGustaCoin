
#pragma once

class EBotSession : public Entity
{
public:
  static const EType eType = EType::botSession;

public:
  enum class State
  {
    stopped = meguco_user_session_stopped,
    starting = meguco_user_session_starting,
    running = meguco_user_session_running,
    stopping = meguco_user_session_stopping,
    //simulating = meguco_user_session_simulating,
  };

  enum class Mode
  {
    live = meguco_user_session_live,
    simulation = meguco_user_session_simulation,
  };

public:
  EBotSession(const QString& name, meguco_user_session_entity& data) : Entity(eType, data.entity.id), name(name)
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

