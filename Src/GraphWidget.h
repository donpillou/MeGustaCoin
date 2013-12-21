
#pragma once

class GraphWidget : public QWidget
{
  Q_OBJECT

public:
  GraphWidget(QWidget* parent, QSettings& settings, DataModel& dataModel);

  void saveState(QSettings& settings);

private slots:
  void setMarket(Market* market);
  void setZoom(int maxTime);

private:
  Market* market;
  DataModel& dataModel;
  GraphModel& graphModel;
  GraphView* graphView;

  QAction* zoomAction;
  QSignalMapper* zoomSignalMapper;
  int zoom;
};

