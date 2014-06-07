
#pragma once

class ELogMessage : public Entity
{
public:
  static const EType eType = EType::logMessage;

public:
  enum class Type
  {
    information,
    warning,
    error,
  };

public:
  ELogMessage(Type type, const QString& message) : Entity(eType, 0)
  {
    this->type = type;
    this->date = QDateTime::currentDateTime();
    this->message = message;
  }

  Type getType() const {return type;}
  const QDateTime& getDate() const {return date;}
  const QString& getMessage() const {return message;}

private:
  Type type;
  QDateTime date;
  QString message;
};
