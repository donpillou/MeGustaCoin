
#pragma once

class BotPropertiesWidget : public QWidget
{
  Q_OBJECT

public:
  BotPropertiesWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager, DataService& dataService);

  void saveState(QSettings& settings);

private slots:
  void editedProperty(const QModelIndex& index, const QString& value);
  void editedProperty(const QModelIndex& index, double value);

private:
  DataService& dataService;
  UserSessionPropertiesModel propertiesModel;
  QTreeView* propertyView;
};
