
#pragma once

class BotsWidget : public QWidget
{
  Q_OBJECT

public:
  BotsWidget(QWidget* parent, QSettings& settings, DataModel& dataModel, BotService& botService);

  void saveState(QSettings& settings);

public slots:
  void optimize();
  void simulate(bool enabled);
  void activate(bool enabled);
  void updateToolBarButtons();

private:
  DataModel& dataModel;
  BotService& botService;

  QSplitter* splitter;
  QTreeView* orderView;
  QTreeView* transactionView;

  QAction* optimizeAction;
  QAction* simulateAction;
  QAction* activateAction;
};
