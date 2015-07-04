
#pragma once

class EBotMarket : public Entity
{
public:
  static const EType eType = EType::botMarket;

public:
  enum class State
  {
    stopped = meguco_user_broker_stopped,
    running = meguco_user_broker_running,
    starting,
  };

public:
  EBotMarket(quint32 tableId, const meguco_user_broker_entity& data) : Entity(eType, tableId)
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

