
#pragma once

class EBotSessionProperty : public Entity
{
public:
  static const EType eType = EType::botSessionProperty;

public:
  enum class Type
  {
    number = BotProtocol::SessionProperty::number,
    string = BotProtocol::SessionProperty::string,
  };

  enum Flag
  {
    none = BotProtocol::SessionProperty::none,
    readOnly = BotProtocol::SessionProperty::readOnly,
  };

public:
  EBotSessionProperty(BotProtocol::SessionProperty& data) : Entity(eType, data.entityId)
  {
    type = (Type)data.type;
    flags = data.flags;
    name = BotProtocol::getString(data.name);
    value = BotProtocol::getString(data.value);
    unit = BotProtocol::getString(data.unit);
  }

  Type getType() const {return type;}
  quint32 getFlags() const {return flags;}
  const QString& getName() const {return name;}
  const QString& getValue() const {return value;}
  const QString& getUnit() const {return unit;}

private:
  Type type;
  quint32 flags;
  QString name;
  QString value;
  QString unit;
};
