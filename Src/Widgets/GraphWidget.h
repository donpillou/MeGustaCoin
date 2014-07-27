
#pragma once

class GraphWidget : public QWidget, public Entity::Listener
{
  Q_OBJECT

public:
  GraphWidget(QTabFramework& tabFramework, QSettings& settings, const QString& channelName, const QString& settingsSection, Entity::Manager& channelEntityManager, GraphModel& graphModel);
  ~GraphWidget();

  void saveState(QSettings& settings);

  void updateTitle();

private slots:
  void setZoom(int maxTime);
  void setEnabledData(int data);
  void updateDataMenu();

private:
  QTabFramework& tabFramework;
  QString channelName;
  QString settingsSection;
  Entity::Manager& channelEntityManager;
  GraphModel& graphModel;
  GraphView* graphView;

  QAction* zoomAction;
  QSignalMapper* zoomSignalMapper;

  QMenu* dataMenu;
  QSignalMapper* dataSignalMapper;

private: // Entity::Listener
  virtual void updatedEntitiy(Entity& oldEntity, Entity& newEntity);
};

