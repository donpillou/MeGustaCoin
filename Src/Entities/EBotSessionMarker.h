
#pragma once

class EBotSessionMarker : public Entity
{
public:
  static const EType eType = EType::userSessionMarker;

public:
  enum class Type
  {
    buy = meguco_user_session_marker_buy,
    sell = meguco_user_session_marker_sell,
    buyAttempt = meguco_user_session_marker_buy_attempt,
    sellAttempt = meguco_user_session_marker_sell_attempt,
    goodBuy = meguco_user_session_marker_good_buy,
    goodSell = meguco_user_session_marker_good_sell,
  };

public:
  EBotSessionMarker(const meguco_user_session_marker_entity& data) : Entity(eType, data.entity.id)
  {
    type = (Type)data.type;
    date = data.entity.time;
  }

  Type getType() const {return type;}
  const qint64& getDate() const {return date;}

private:
  Type type;
  qint64 date;
};
