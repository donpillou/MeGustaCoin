
#pragma once

class EBotSessionProperty : public Entity
{
public:
  static const EType eType = EType::botSessionProperty;

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
  EBotSessionProperty(const meguco_user_session_property_entity& data) : Entity(eType, data.entity.id)
  {
    type = (Type)data.type;
    flags = data.flags;
    DataConnection::getString(data.entity, sizeof(data), data.name_size, name);
    DataConnection::getString(data.entity, sizeof(data) + data.name_size, data.value_size, value );
    DataConnection::getString(data.entity, sizeof(data) + data.name_size + data.value_size, data.unit_size, unit);
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
