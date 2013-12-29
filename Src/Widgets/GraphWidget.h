
#pragma once

class GraphWidget : public QWidget
{
  Q_OBJECT

public:
  GraphWidget(QWidget* parent, QSettings& settings, DataModel& dataModel);

  void saveState(QSettings& settings);

private slots:
  void setZoom(int maxTime);
  void setEnabledData(int data);

private:
  DataModel& dataModel;
  GraphModel& graphModel;
  GraphView* graphView;

  QAction* zoomAction;
  QSignalMapper* zoomSignalMapper;
  int zoom;
};

