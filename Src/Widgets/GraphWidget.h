
#pragma once

class GraphWidget : public QWidget
{
  Q_OBJECT

public:
  GraphWidget(QWidget* parent, QSettings& settings, PublicDataModel& publicDataModel, const QMap<QString, PublicDataModel*>& publicDataModels);

  void saveState(QSettings& settings);

private slots:
  void setZoom(int maxTime);
  void setEnabledData(int data);

private:
  PublicDataModel& publicDataModel;
  GraphModel& graphModel;
  GraphView* graphView;

  QAction* zoomAction;
  QSignalMapper* zoomSignalMapper;
};

