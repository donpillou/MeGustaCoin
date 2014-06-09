
#pragma once

class GraphWidget : public QWidget, public Entity::Listener
{
  Q_OBJECT

public:
  GraphWidget(QWidget* parent, QSettings& settings, const QString& channelName, const QString& settingsSection, Entity::Manager& globalEntityManager, Entity::Manager& channelEntityManager, const GraphModel& graphModel, const QMap<QString, GraphModel*>& graphModels);
  ~GraphWidget();

  void saveState(QSettings& settings);

  //void setFocusPublicDataModel(const PublicDataModel* publicDataModel);
  //const PublicDataModel* getFocusPublicDataModel() const {return publicDataModel;}

  void updateTitle();

private slots:
  void setZoom(int maxTime);
  void setEnabledData(int data);
  void updateDataMenu();

private:
  QString channelName;
  QString settingsSection;
  Entity::Manager& channelEntityManager;
  GraphView* graphView;

  QAction* zoomAction;
  QSignalMapper* zoomSignalMapper;

  QMenu* dataMenu;
  QSignalMapper* dataSignalMapper;

private: // Entity::Listener
  virtual void updatedEntitiy(Entity& oldEntity, Entity& newEntity);
};

