
#pragma once

class EBotMarket : public Entity // todo: rename EUserBroker
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
    brokerTypeId = data.broker_type_id;
    state = (State)data.state;
  }

  quint64 getBrokerTypeId() const {return brokerTypeId;}
  State getState() const {return state;}

private:
  quint64 brokerTypeId;
  State state;
};

