
#pragma once

class EUserBroker : public Entity
{
public:
  static const EType eType = EType::userBroker;

public:
  enum class State
  {
    stopped = meguco_user_broker_stopped,
    running = meguco_user_broker_running,
    starting,
  };

public:
  EUserBroker(quint32 tableId, const meguco_user_broker_entity& data, const QString& userName) : Entity(eType, tableId), userName(userName)
  {
    brokerTypeId = data.broker_type_id;
    state = (State)data.state;
  }

  quint64 getBrokerTypeId() const {return brokerTypeId;}
  State getState() const {return state;}
  const QString& getUserName() const {return userName;}

private:
  quint64 brokerTypeId;
  State state;
  QString userName;
};

