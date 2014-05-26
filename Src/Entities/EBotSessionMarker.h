
#pragma once

class EBotSessionMarker : public Entity
{
public:
  static const EType eType = EType::botSessionMarker;

public:
  enum class Type
  {
    buy = BotProtocol::Marker::buy,
    sell = BotProtocol::Marker::sell,
    buyAttempt = BotProtocol::Marker::buyAttempt,
    sellAttempt = BotProtocol::Marker::sellAttempt,
  };

public:
  EBotSessionMarker(BotProtocol::Marker& data) : Entity(eType, data.entityId)
  {
    type = (Type)data.type;
    date = data.date;
  }

  Type getType() const {return type;}
  const qint64& getDate() const {return date;}

private:
  Type type;
  qint64 date;
};
