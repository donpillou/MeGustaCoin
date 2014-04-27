
#pragma once

class EBotEngine : public Entity
{
public:
  static const EType eType = EType::botEngine;

public:
  EBotEngine(BotProtocol::BotEngine& data) : Entity(eType, data.entityId)
  {
    data.name[sizeof(data.name) - 1] = '\0';
    name = data.name;
  }

  const QString& getName() const {return name;}

private:
  QString name;
};
