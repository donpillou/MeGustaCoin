
#pragma once

class EBotEngine : public Entity
{
public:
  static const EType eType = EType::botEngine;

public:
  EBotEngine(BotProtocol::BotEngine& data) : Entity(eType, data.entityId)
  {
    name = BotProtocol::getString(data.name);
  }

  const QString& getName() const {return name;}

private:
  QString name;
};
