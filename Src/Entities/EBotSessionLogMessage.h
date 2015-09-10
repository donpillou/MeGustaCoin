
#pragma once

class EBotSessionLogMessage : public Entity
{
public:
  static const EType eType = EType::botSessionLogMessage;

public:
  EBotSessionLogMessage(const meguco_log_entity& data, const QString& message) : Entity(eType, data.entity.id), message(message)
  {
    date = QDateTime::fromMSecsSinceEpoch(data.entity.time);
  }

  const QDateTime& getDate() const {return date;}
  const QString& getMessage() const {return message;}

private:
  QDateTime date;
  QString message;
};
