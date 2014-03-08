
#include "stdafx.h"

BotsWidget::BotsWidget(QWidget* parent, QSettings& settings, DataModel& dataModel, MarketService& marketService) :
  QWidget(parent), dataModel(dataModel),  marketService(marketService), thread(0)
{
  //connect(&dataModel.orderModel, SIGNAL(changedState()), this, SLOT(updateTitle()));
  //connect(&dataModel, SIGNAL(changedMarket()), this, SLOT(updateToolBarButtons()));

  botsModel.addBot("BuyBot", *new BuyBot);

  QToolBar* toolBar = new QToolBar(this);
  toolBar->setStyleSheet("QToolBar { border: 0px }");
  toolBar->setIconSize(QSize(16, 16));
  toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  QAction* simulateAction = toolBar->addAction(QIcon(":/Icons/chart_curve.png"), tr("&Simulate"));
  //simulateAction->setEnabled(false);
  connect(simulateAction, SIGNAL(triggered()), this, SLOT(simulate()));

  /*
  textEdit = new QTextEdit(this);
  QFont font("");
  font.setStyleHint(QFont::TypeWriter);
  textEdit->setFont(font);
  textEdit->setLineWrapMode(QTextEdit::NoWrap);
  textEdit->setAcceptRichText(false);
  */
  botsView = new QTreeView(this);
  botsView->setUniformRowHeights(true);
  botsView->setModel(&botsModel);
  //logView->setSortingEnabled(true);
  botsView->setRootIsDecorated(false);
  botsView->setAlternatingRowColors(true);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->addWidget(toolBar);
  layout->addWidget(botsView);
  setLayout(layout);

  QHeaderView* headerView = botsView->header();
  headerView->resizeSection(0, 300);
  headerView->resizeSection(1, 110);
  headerView->setStretchLastSection(false);
  headerView->setResizeMode(0, QHeaderView::Stretch);
  settings.beginGroup("Bots");
  headerView->restoreState(settings.value("HeaderState").toByteArray());
  settings.endGroup();
}

BotsWidget::~BotsWidget()
{
  if(thread)
  {
    thread->wait();
    delete thread;
    thread = 0;
    qDeleteAll(actionQueue.getAll());
  }
}

void BotsWidget::saveState(QSettings& settings)
{
  settings.beginGroup("Bots");
  settings.setValue("HeaderState", botsView->header()->saveState());
  settings.endGroup();
}

QList<QModelIndex> BotsWidget::getSelectedRows()
{ // since orderView->selectionModel()->selectedRows(); does not work
  QList<QModelIndex> result;
  QItemSelection selection = botsView->selectionModel()->selection();
  foreach(const QItemSelectionRange& range, selection)
  {
    QModelIndex parent = range.parent();
    for(int i = range.top(), end = range.bottom() + 1; i < end; ++i)
      result.append(range.model()->index(i, 0, parent));
  }
  return result;
}

void BotsWidget::simulate()
{
  if(thread)
    return;

  // get selected bot
  QList<QModelIndex> seletedIndices = getSelectedRows();
  Bot* botFactory = 0;
  foreach(const QModelIndex& index, seletedIndices)
  {
    botFactory = botsModel.getBotFactory(index);
    break;
  }
  if(!botFactory)
    return;

  // start thread
  thread = new Thread(*this, *botFactory, dataModel.getMarketName());
  thread->start();
}

void BotsWidget::Thread::run()
{
  // connect to data service
  DataConnection dataConnection;
  logMessage(LogModel::Type::information, "Connecting to data service...");
  if(!dataConnection.connect())
  {
    quit(LogModel::Type::error, QString("Could not connect to data service: %1").arg(dataConnection.getLastError()));
    return;
  }
  logMessage(LogModel::Type::information, "Connected to data service.");

  // load data
  logMessage(LogModel::Type::information, "Loading trade data...");
  if(!dataConnection.subscribe(marketName, 0))
  {
    quit(LogModel::Type::error, QString("Lost connection to data service: %1").arg(dataConnection.getLastError()));
    return;
  }
  QList<DataProtocol::Trade> trades;
  DataProtocol::Trade trade;
  quint64 channelId;
  for(;;)
  {
    if(!dataConnection.readTrade(channelId, trade))
    {
      quit(LogModel::Type::error, QString("Lost connection to data service: %1").arg(dataConnection.getLastError()));
      return;
    }
    trades.append(trade);
    if(!(trade.flags & DataProtocol::replayedFlag) || trade.flags & DataProtocol::syncFlag)
      break;
  }
  logMessage(LogModel::Type::information, "Loaded trade data.");

  // create simulation market
  class SimMarket : public Bot::Market
  {
    virtual bool buy(double price, double amount, quint64 timeout)
    {
      return false;
    };
    virtual bool sell(double price, double amount, quint64 timeout)
    {
      return false;
    };
  } simMarket;

  // create simulation agent
  Bot::Session* botSession = botFactory.createSession(simMarket);
  if(!botSession)
  {
    quit(LogModel::Type::error, "Could not create bot session.");
    return;
  }

  // run simulation
  TradeHandler tradeHandler;
  for(QList<DataProtocol::Trade>::ConstIterator i = trades.begin(), end = trades.end(); i != end; ++i)
  {
    const DataProtocol::Trade& trade = *i;
    tradeHandler.add(trade, true);
    botSession->handle(trade, tradeHandler.values);
  }

  // done
  delete botSession;
  quit(LogModel::Type::information, "Completed simulation.");
}

void BotsWidget::Thread::logMessage(LogModel::Type type, const QString& message)
{
  class MessageAction : public Action
  {
  public:
    LogModel::Type type;
    QString message;
    MessageAction(LogModel::Type type, const QString& message) : type(type), message(message) {}
    virtual void execute(BotsWidget& widget)
    {
      widget.dataModel.logModel.addMessage(type, message);
    }
  };
  widget.actionQueue.append(new MessageAction(type, message));
  QTimer::singleShot(0, &widget, SLOT(executeActions()));
}

void BotsWidget::Thread::quit(LogModel::Type type, const QString& message)
{
  class QuitAction : public Action
  {
  public:
    LogModel::Type type;
    QString message;
    QuitAction(LogModel::Type type, const QString& message) : type(type), message(message) {}
    virtual void execute(BotsWidget& widget)
    {
      widget.dataModel.logModel.addMessage(type, message);
      if(widget.thread)
      {
        widget.thread->wait();
        delete widget.thread;
        widget.thread = 0;
      }
    }
  };
  widget.actionQueue.append(new QuitAction(type, message));
  QTimer::singleShot(0, &widget, SLOT(executeActions()));
}

void BotsWidget::executeActions()
{
  for(;;)
  {
    Action* action = 0;
    if(!actionQueue.get(action, 0) || !action)
      break;
    action->execute(*this);
    delete action;
  }
}
