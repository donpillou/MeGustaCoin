
#pragma once

class EBotSessionLogMessage : public Entity
{
public:
  static const EType eType = EType::botSessionLogMessage;

public:
  EBotSessionLogMessage(BotProtocol::SessionLogMessage& data) : Entity(eType, data.entityId)
  {
    date = QDateTime::fromMSecsSinceEpoch(data.date);
    message = BotProtocol::getString(data.message);
  }

  const QDateTime& getDate() const {return date;}
  const QString& getMessage() const {return message;}

private:
  QDateTime date;
  QString message;
};
