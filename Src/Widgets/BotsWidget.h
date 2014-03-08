
#pragma once

class BotsWidget : public QWidget
{
  Q_OBJECT

public:
  BotsWidget(QWidget* parent, QSettings& settings, DataModel& dataModel, MarketService& marketService);
  ~BotsWidget();

  void saveState(QSettings& settings);

public slots:
  void simulate();

private:
  class Action
  {
  public:
    virtual void execute(BotsWidget& widget) = 0;
  };

  class Thread : public QThread
  {
  public:
    Thread(BotsWidget& widget, Bot& botFactory, const QString& marketName) : widget(widget), botFactory(botFactory), marketName(marketName) {}

  private:
    BotsWidget& widget;
    Bot& botFactory;
    QString marketName;

    virtual void run();

    void logMessage(LogModel::Type type, const QString& message);
    void quit(LogModel::Type type, const QString& message);
  };

  DataModel& dataModel;
  MarketService& marketService;
  BotsModel botsModel;

  QThread* thread;
  JobQueue<Action*> actionQueue;

  QTreeView* botsView;

  QList<QModelIndex> getSelectedRows();

private slots:
  void executeActions();
};
