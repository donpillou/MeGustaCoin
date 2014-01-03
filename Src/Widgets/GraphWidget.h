
#pragma once

class GraphWidget : public QWidget
{
  Q_OBJECT

public:
  GraphWidget(QWidget* parent, QSettings& settings, DataModel& dataModel, const QString& focusMarketName, const QString& graphNum, const QMap<QString, PublicDataModel*>& publicDataModels);

  void saveState(QSettings& settings);

private slots:
  void setZoom(int maxTime);
  void setEnabledData(int data);
  void updateDataMenu();

private:
  DataModel& dataModel;
  QString focusMarketName;
  QString graphNum;
  const QMap<QString, PublicDataModel*>& publicDataModels;
  GraphView* graphView;

  QAction* zoomAction;
  QSignalMapper* zoomSignalMapper;

  QMenu* dataMenu;
  QSignalMapper* dataSignalMapper;
};

