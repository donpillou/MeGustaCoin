
#pragma once

class EBotEngine : public Entity
{
public:
  static const EType eType = EType::botEngine;

public:
  EBotEngine(meguco_bot_engine_entity& data) : Entity(eType, data.entity.id)
  {
    name = DataConnection::getString(data.entity, sizeof(data), data.name_size);
  }

  const QString& getName() const {return name;}

private:
  QString name;
};
