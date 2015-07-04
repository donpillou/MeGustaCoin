
#pragma once

class EBotSessionLogMessage : public Entity
{
public:
  static const EType eType = EType::botSessionLogMessage;

public:
  EBotSessionLogMessage(const meguco_log_entity& data) : Entity(eType, data.entity.id)
  {
    date = QDateTime::fromMSecsSinceEpoch(data.entity.time);
    DataConnection::getString(data.entity, sizeof(data), data.message_size, message);
  }

  const QDateTime& getDate() const {return date;}
  const QString& getMessage() const {return message;}

private:
  QDateTime date;
  QString message;
};
