
#pragma once

class EProcess : public Entity
{
public:
  static const EType eType = EType::process;

public:
  EProcess(const meguco_process_entity& data, const QString& cmd) : Entity(eType, data.entity.id), cmd(cmd) {}

  const QString& getCmd() const {return cmd;}

private:
  QString cmd;
};
