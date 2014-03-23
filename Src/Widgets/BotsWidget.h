
#pragma once

class BotsWidget : public QWidget
{
  Q_OBJECT

public:
  BotsWidget(QWidget* parent, QSettings& settings, DataModel& dataModel, BotService& botService);

  void saveState(QSettings& settings);

private slots:
  void addBot();
  void optimize();
  void simulate(bool enabled);
  void activate(bool enabled);
  void updateTitle();
  void updateToolBarButtons();

private:
  DataModel& dataModel;
  BotService& botService;

  QSplitter* splitter;
  QTreeView* orderView;
  QTreeView* transactionView;

  QAction* addAction;
  QAction* optimizeAction;
  QAction* simulateAction;
  QAction* activateAction;
};
