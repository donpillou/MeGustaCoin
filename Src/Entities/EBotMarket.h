
#pragma once

class EBotMarket : public Entity
{
public:
  static const EType eType = EType::botMarket;

public:
  enum class State
  {
    stopped = meguco_user_market_stopped,
    starting = meguco_user_market_starting,
    running = meguco_user_market_running,
  };

public:
  EBotMarket(meguco_user_market_entity& data) : Entity(eType, data.entity.id)
  {
    marketAdapterId = data.bot_market_id;
    state = (State)data.state;
  }

  quint64 getMarketAdapterId() const {return marketAdapterId;}
  State getState() const {return state;}

private:
  quint64 marketAdapterId;
  State state;
};

