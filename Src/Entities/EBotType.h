
#pragma once

class EBotType : public Entity
{
public:
  static const EType eType = EType::botEngine;

public:
  EBotType(const meguco_bot_type_entity& data) : Entity(eType, data.entity.id)
  {
    DataConnection::getString(data.entity, sizeof(data), data.name_size, name);
  }

  const QString& getName() const {return name;}

private:
  QString name;
};
