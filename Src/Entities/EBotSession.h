
#pragma once

class EBotSession : public Entity
{
public:
  static const EType eType = EType::botSession;

public:
  enum class State
  {
    stopped = BotProtocol::Session::stopped,
    starting = BotProtocol::Session::starting,
    running = BotProtocol::Session::running,
    simulating = BotProtocol::Session::simulating,
  };

public:
  EBotSession(quint32 id, BotProtocol::Session& data) : Entity(eType, id)
  {
    data.name[sizeof(data.name) - 1] = '\0';
    name = data.name;
    engineId = data.engineId;
    marketId = data.marketId;
    state = (State)data.state;
  }

  const QString& getName() const {return name;}
  quint32 getEngineId() const {return engineId;}
  quint32 getMarketId() const {return marketId;}
  State getState() const {return state;}

private:
  QString name;
  quint32 engineId;
  quint32 marketId;
  State state;
};

