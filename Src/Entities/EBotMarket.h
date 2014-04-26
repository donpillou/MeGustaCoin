
#pragma once

class EBotMarket : public Entity
{
public:
  static const EType eType = EType::botMarket;

public:
  enum class State
  {
    stopped = BotProtocol::Market::stopped,
    running = BotProtocol::Market::running,
  };

public:
  EBotMarket(quint32 id, BotProtocol::Market& data) : Entity(eType, id)
  {
    marketAdapterId = data.marketAdapterId;
    state = (State)data.state;
  }

  quint32 getMarketAdapterId() const {return marketAdapterId;}
  State getState() const {return state;}

private:
  quint32 marketAdapterId;
  State state;
};

