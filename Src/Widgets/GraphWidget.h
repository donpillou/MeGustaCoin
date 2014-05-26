
#pragma once

class GraphWidget : public QWidget
{
  Q_OBJECT

public:
  GraphWidget(QWidget* parent, QSettings& settings, const PublicDataModel* publicDataModel, const QString& graphNum, const QMap<QString, PublicDataModel*>& publicDataModels, Entity::Manager& entityManager);

  void saveState(QSettings& settings);

  void setFocusPublicDataModel(const PublicDataModel* publicDataModel);
  const PublicDataModel* getFocusPublicDataModel() const {return publicDataModel;}

public slots:
  void updateTitle();

private slots:
  void setZoom(int maxTime);
  void setEnabledData(int data);
  void updateDataMenu();

private:
  const PublicDataModel* publicDataModel;
  QString settingsSection;
  const QMap<QString, PublicDataModel*>& publicDataModels;
  GraphView* graphView;

  QAction* zoomAction;
  QSignalMapper* zoomSignalMapper;

  QMenu* dataMenu;
  QSignalMapper* dataSignalMapper;
};

