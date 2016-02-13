
#pragma once

class EBotSessionProperty : public Entity
{
public:
  static const EType eType = EType::userSessionProperty;

public:
  enum class Type
  {
    number = meguco_user_session_property_number,
    string = meguco_user_session_property_string,
  };

  enum Flag
  {
    none = meguco_user_session_property_none,
    readOnly = meguco_user_session_property_read_only,
  };

public:
  EBotSessionProperty(const meguco_user_session_property_entity& data, const QString& name, const QString& value, const QString& unit) : Entity(eType, data.entity.id), name(name), value(value), unit(unit)
  {
    type = (Type)data.type;
    flags = data.flags;
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
