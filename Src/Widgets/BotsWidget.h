
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
  void activate(bool enabled);
  void updateToolBarButtons();

private:
  class Action
  {
  public:
    virtual void execute(BotsWidget& widget) = 0;
  };

  class Thread : public QThread
  {
  public:
    Thread(BotsWidget& widget, Bot& botFactory, const QString& marketName, double fee) : widget(widget), botFactory(botFactory), marketName(marketName), fee(fee) {}

  private:
    BotsWidget& widget;
    Bot& botFactory;
    QString marketName;
    double fee;

    virtual void run();

    void logMessage(LogModel::Type type, const QString& message);
    void quit(LogModel::Type type, const QString& message);
    void clearMarkers();
    void addMarker(quint64 time, GraphModel::Marker marker);
  };

  DataModel& dataModel;
  MarketService& marketService;
  BotService botService;

  QThread* thread;
  JobQueue<Action*> actionQueue;

  QSplitter* splitter;
  QTreeView* orderView;
  QTreeView* transactionView;

  QAction* simulateAction;
  QAction* activateAction;

  //QList<QModelIndex> getSelectedRows();

private slots:
  void executeActions();
};
