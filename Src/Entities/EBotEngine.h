
#pragma once

class EBotEngine : public Entity
{
public:
  static const EType eType = EType::botEngine;

public:
  EBotEngine(quint32 id, BotProtocol::Engine& data) : Entity(eType, id)
  {
    data.name[sizeof(data.name) - 1] = '\0';
    name = data.name;
  }

  const QString& getName() const {return name;}

private:
  QString name;
};
