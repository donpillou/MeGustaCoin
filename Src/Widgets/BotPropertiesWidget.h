
#pragma once

class BotPropertiesWidget : public QWidget
{
public:
  BotPropertiesWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager);

  void saveState(QSettings& settings);

private:
  QTabFramework& tabFramework;
  Entity::Manager& entityManager;

  SessionPropertyModel propertyModel;

  QTreeView* propertyView;
};
