
#pragma once

class EUserSession : public Entity
{
public:
  static const EType eType = EType::userSession;

public:
  enum class State
  {
    stopped = meguco_user_session_stopped,
    starting = meguco_user_session_starting,
    stopping = meguco_user_session_stopping,
    running = meguco_user_session_running,
  };

  enum class Mode
  {
    none = meguco_user_session_none,
    live = meguco_user_session_live,
    simulation = meguco_user_session_simulation,
  };

public:
  EUserSession(quint32 tableId, const QString& name, const meguco_user_session_entity& data) : Entity(eType, tableId), name(name)
  {
    botTypeId = data.bot_type_id;
    brokerId = data.broker_id;
    state = (State)data.state;
    mode = (Mode)data.mode;
  }

  const QString& getName() const {return name;}
  quint64 getBotTypeId() const {return botTypeId;}
  quint64 getBrokerId() const {return brokerId;}
  State getState() const {return state;}
  Mode getMode() const {return mode;}

private:
  QString name;
  quint64 botTypeId;
  quint64 brokerId;
  State state;
  Mode mode;
};

