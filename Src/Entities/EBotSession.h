
#pragma once

class EBotSession : public Entity
{
public:
  static const EType eType = EType::botSession;

  EBotSession(quint32 id, BotProtocol::Session& data) : Entity(eType, id)
  {
    data.name[sizeof(data.name) - 1] = '\0';
    data.engine[sizeof(data.engine) - 1] = '\0';
    name = data.name;
    engine = data.engine;
  }

  const QString& getName() const {return name;}
  const QString& getEngine() const {return engine;}

private:
  QString name;
  QString engine;
};

