
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
  EBotSession(BotProtocol::Session& data) : Entity(eType, data.entityId)
  {
    name = BotProtocol::getString(data.name);
    botEngineId = data.botEngineId;
    marketId = data.marketId;
    state = (State)data.state;
  }

  const QString& getName() const {return name;}
  quint32 getEngineId() const {return botEngineId;}
  quint32 getMarketId() const {return marketId;}
  State getState() const {return state;}

private:
  QString name;
  quint32 botEngineId;
  quint32 marketId;
  State state;
};

