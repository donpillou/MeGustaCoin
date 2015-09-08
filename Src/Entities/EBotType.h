
#pragma once

class EBotType : public Entity
{
public:
  static const EType eType = EType::botType;

public:
  EBotType(const meguco_bot_type_entity& data, const QString& name) : Entity(eType, data.entity.id), name(name) {}

  const QString& getName() const {return name;}

private:
  QString name;
};
