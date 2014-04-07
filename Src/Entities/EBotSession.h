
#pragma once

class EBotSession : public Entity
{
public:
  static const EType eType = EType::botSession;

public:
  enum class State
  {
    inactive = BotProtocol::Session::inactive,
    active = BotProtocol::Session::active,
    simulating = BotProtocol::Session::simulating,
  };

public:
  EBotSession(quint32 id, BotProtocol::Session& data) : Entity(eType, id)
  {
    data.name[sizeof(data.name) - 1] = '\0';
    data.engine[sizeof(data.engine) - 1] = '\0';
    name = data.name;
    engine = data.engine;
    state = (State)data.state;
  }

  const QString& getName() const {return name;}
  const QString& getEngine() const {return engine;}
  State getState() const {return state;}

private:
  QString name;
  QString engine;
  State state;
};

