
#pragma once

class EUserSessionLogMessage : public Entity
{
public:
  static const EType eType = EType::userSessionLogMessage;

public:
  EUserSessionLogMessage(const meguco_log_entity& data, const QString& message) : Entity(eType, data.entity.id), message(message)
  {
    date = QDateTime::fromMSecsSinceEpoch(data.entity.time);
  }

  const QDateTime& getDate() const {return date;}
  const QString& getMessage() const {return message;}

private:
  QDateTime date;
  QString message;
};
