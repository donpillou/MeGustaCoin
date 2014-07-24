
#pragma once

class BotPropertiesWidget : public QWidget
{
  Q_OBJECT

public:
  BotPropertiesWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager, BotService& botService);

  void saveState(QSettings& settings);

private slots:
  void editedProperty(const QModelIndex& index, const QString& value);
  void editedProperty(const QModelIndex& index, double value);

private:
  QTabFramework& tabFramework;
  Entity::Manager& entityManager;
  BotService& botService;

  SessionPropertyModel propertyModel;

  QTreeView* propertyView;
};
