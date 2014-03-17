
#include "stdafx.h"

BotsWidget::BotsWidget(QWidget* parent, QSettings& settings, DataModel& dataModel, MarketService& marketService) :
  QWidget(parent), dataModel(dataModel),  marketService(marketService),
  botService(dataModel), thread(0)
{
  //connect(&dataModel.orderModel, SIGNAL(changedState()), this, SLOT(updateTitle()));
  connect(&dataModel, SIGNAL(changedMarket()), this, SLOT(updateToolBarButtons()));

  //botsModel.addBot("BuyBot", *new BuyBot);

  QToolBar* toolBar = new QToolBar(this);
  toolBar->setStyleSheet("QToolBar { border: 0px }");
  toolBar->setIconSize(QSize(16, 16));
  toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  optimizeAction = toolBar->addAction(QIcon(":/Icons/chart_curve.png"), tr("&Optimize"));
  optimizeAction->setEnabled(false);
  connect(optimizeAction, SIGNAL(triggered()), this, SLOT(optimize()));

  simulateAction = toolBar->addAction(QIcon(":/Icons/user_gray_go_gray.png"), tr("&Simulate"));
  simulateAction->setEnabled(false);
  simulateAction->setCheckable(true);
  connect(simulateAction, SIGNAL(triggered(bool)), this, SLOT(simulate(bool)));

  activateAction = toolBar->addAction(QIcon(":/Icons/user_gray_go.png"), tr("&Activate"));
  activateAction->setEnabled(false);
  activateAction->setCheckable(true);
  connect(activateAction, SIGNAL(triggered(bool)), this, SLOT(activate(bool)));

  orderView = new QTreeView(this);
  orderView->setUniformRowHeights(true);
  OrderSortProxyModel* proxyModel = new OrderSortProxyModel(this, dataModel.botOrderModel);
  proxyModel->setDynamicSortFilter(true);
  proxyModel->setSourceModel(&dataModel.botOrderModel);
  orderView->setModel(proxyModel);
  orderView->setSortingEnabled(true);
  orderView->setRootIsDecorated(false);
  orderView->setAlternatingRowColors(true);
  orderView->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::DoubleClicked);
  orderView->setSelectionMode(QAbstractItemView::ExtendedSelection);

  transactionView = new QTreeView(this);
  transactionView->setUniformRowHeights(true);
  TransactionSortProxyModel* tproxyModel = new TransactionSortProxyModel(this, dataModel.botTransactionModel);
  tproxyModel->setDynamicSortFilter(true);
  tproxyModel->setSourceModel(&dataModel.botTransactionModel);
  transactionView->setModel(tproxyModel);
  transactionView->setSortingEnabled(true);
  transactionView->setRootIsDecorated(false);
  transactionView->setAlternatingRowColors(true);

  splitter = new QSplitter(Qt::Vertical, this);
  splitter->setHandleWidth(1);
  splitter->addWidget(orderView);
  splitter->addWidget(transactionView);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->addWidget(toolBar);
  layout->addWidget(splitter);
  setLayout(layout);

  //connect(botsView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), this, SLOT(updateToolBarButtons()));

  //QHeaderView* headerView = botsView->header();
  //headerView->resizeSection(0, 300);
  //headerView->resizeSection(1, 110);
  //headerView->setStretchLastSection(false);
  //headerView->setResizeMode(0, QHeaderView::Stretch);
  //settings.beginGroup("Bots");
  //headerView->restoreState(settings.value("HeaderState").toByteArray());
  //settings.endGroup();
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
  //settings.setValue("HeaderState", botsView->header()->saveState());
  settings.endGroup();
}

void BotsWidget::activate(bool enable)
{
  if(enable)
    startWorkerThread(false);
  else
    stopWorkerThread();
}

void BotsWidget::simulate(bool enable)
{
  if(enable)
    startWorkerThread(true);
  else
    stopWorkerThread();
}

void BotsWidget::startWorkerThread(bool simulation)
{
    PublicDataModel* publicDataModel = dataModel.getPublicDataModel();
    if(publicDataModel)
      publicDataModel->graphModel.clearMarkers();

    QString userName, key, secret;
    dataModel.getLoginData(userName, key, secret);
    botService.start("BuyBot", simulation, dataModel.getMarketName(), userName, key, secret);
}

void BotsWidget::stopWorkerThread()
{
    botService.stop();
    dataModel.botOrderModel.reset();
    dataModel.botTransactionModel.reset();
    PublicDataModel* publicDataModel = dataModel.getPublicDataModel();
    if(publicDataModel)
      publicDataModel->graphModel.clearMarkers();
}

void BotsWidget::optimize()
{
  if(thread)
    return;

  // get selected bot
  //QList<QModelIndex> seletedIndices = getSelectedRows();
  //Bot* botFactory = 0;
  //foreach(const QModelIndex& index, seletedIndices)
  //{
  //  botFactory = botsModel.getBotFactory(index);
  //  break;
  //}
  //if(!botFactory)
  //  return;
  Bot* botFactory = new BuyBot(); // TODO: fix memory leak

  // start thread
  thread = new Thread(*this, *botFactory, dataModel.getMarketName(), dataModel.getBalance().fee);
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
  logMessage(LogModel::Type::information, QString("Loading %1 trade data...").arg(marketName));
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


  unsigned int parameterCount = botFactory.getParameterCount();
  double* parameters = (double*)alloca(sizeof(double) * parameterCount);
  double* bestParameters = (double*)alloca(sizeof(double) * parameterCount);
  memset(parameters, 0, sizeof(double) * parameterCount);
  ParticleSwarm particleSwarm(30); //, 0.006f, 0.008f, 0.01f, 0.0002f); //, 0.6f, 0.8f, 1.f, 0.6f);
  for(unsigned int i = 0; i < parameterCount; ++i)
    particleSwarm.addDimension(parameters[i], 0., 1.);
  particleSwarm.start();

  double bestRating = 0.;
  unsigned int iterations = 1000;
  for(unsigned int i = 0; i < iterations; ++i)
  {
    particleSwarm.next();

    // create simulation market
    class SimBroker : public Bot::Broker
    {
    public:
      Bot::Session* session;

      SimBroker(Thread& thread, double balanceBase, double balanceComm, double fee) : session(0), thread(thread), lastBuyTime(0), lastSellTime(0), balanceBase(balanceBase), balanceComm(balanceComm), fee(fee), nextTransactionId(1) {}

      void cancelAllOrders()
      {
        for(int i = 0; i < buyOrders.size(); ++i)
        {
          const Order& order = buyOrders[i];
          balanceBase += order.amount * order.price + order.fee;
        }
        buyOrders.clear();
        for(int i = 0; i < sellOrders.size(); ++i)
        {
          const Order& order = sellOrders[i];
          balanceComm += order.amount;
        }
        sellOrders.clear();
      }

      void update(const DataProtocol::Trade& trade)
      {
        time = trade.time / 1000;

        for(int i = 0; i < buyOrders.size();)
        {
          const Order& order = buyOrders[i];
          if(time >= order.timeout)
          {
            balanceBase += order.amount * order.price + order.fee;
            buyOrders.removeAt(i);
          }
          else if(trade.price < order.price)
          {
            thread.addMarker(time, GraphModel::Marker::buyMarker);
            thread.logMessage(LogModel::Type::information, QString("Bought %1 @ %2.").arg(DataModel::formatAmount(order.amount), DataModel::formatPrice(order.price)));

            Transaction transaction;
            transaction.id = nextTransactionId++;
            transaction.price  = order.price;
            transaction.amount = order.amount;
            transaction.fee = order.fee;
            transaction.type = Transaction::Type::buy;
            transactions.insert(transaction.id, transaction);

            lastBuyTime = time;
            balanceComm += order.amount;

            buyOrders.removeAt(i);
            session->handleBuy(transaction);
          }
          else
            ++i;
        }

        for(int i = 0; i < sellOrders.size();)
        {
          const Order& order = sellOrders[i];
          if(time >= order.timeout)
          {
            balanceComm += order.amount;
            sellOrders.removeAt(i);
          }
          else if(trade.price > order.price)
          {
            thread.addMarker(time, GraphModel::Marker::sellMarker);
            thread.logMessage(LogModel::Type::information, QString("Sold %1 @ %2.").arg(DataModel::formatAmount(order.amount), DataModel::formatPrice(order.price)));

            Transaction transaction;
            transaction.id = nextTransactionId++;
            transaction.price  = order.price;
            transaction.amount = order.amount;
            transaction.fee = order.fee;
            transaction.type = Transaction::Type::sell;
            transactions.insert(transaction.id, transaction);

            lastSellTime = time;
            balanceBase += order.amount * order.price - transaction.fee;

            sellOrders.removeAt(i);
            session->handleSell(transaction);
          }
          else
            ++i;
        }
      }

      void getBalance(double& base, double& comm) const
      {
        base = balanceBase;
        comm = balanceComm;
      }

    private:
      struct Order
      {
        double price;
        double amount;
        double fee;
        quint64 timeout;
      };

      Thread& thread;

      quint64 time;

      QList<Order> buyOrders;
      QList<Order> sellOrders;
      quint64 lastBuyTime;
      quint64 lastSellTime;

      double balanceBase; // USD
      double balanceComm; // BTC
      double fee;

      QMap<quint64, Transaction> transactions;
      quint64 nextTransactionId;

    private:
      virtual bool buy(double price, double amount, quint64 timeout)
      {
        double fee = ceil(amount * price * this->fee * 100.) / 100.;
        double charge = amount * price + fee;
        if(charge > balanceBase)
          return false;

        Order order = {price, amount, fee, time + timeout};
        buyOrders.append(order);
        balanceBase -= charge;
        thread.addMarker(time, GraphModel::Marker::buyAttemptMarker);
        return true;
      };
      virtual bool sell(double price, double amount, quint64 timeout)
      {
        if(amount > balanceComm)
          return false;

        double fee = ceil(amount * price * this->fee * 100.) / 100.;
        Order order = {price, amount, fee, time + timeout};
        sellOrders.append(order);
        balanceComm -= amount;
        thread.addMarker(time, GraphModel::Marker::sellAttemptMarker);
        return true;
      };
      virtual double getBalanceBase() const {return balanceBase;}
      virtual double getBalanceComm() const {return balanceComm;}
      virtual double getFee() const {return fee;}
      virtual unsigned int getOpenBuyOrderCount() const {return buyOrders.size();}
      virtual unsigned int getOpenSellOrderCount() const {return sellOrders.size();}
      virtual quint64 getTimeSinceLastBuy() const {return time - lastBuyTime;}
      virtual quint64 getTimeSinceLastSell() const {return time - lastSellTime;}
      virtual void warning(const QString& message) {thread.logMessage(LogModel::Type::warning, message);}

      virtual void getTransactions(QList<Transaction>& transactions) const
      {
        foreach(const Transaction& transaction, this->transactions)
          transactions.append(transaction);
      }

      virtual void getBuyTransactions(QList<Transaction>& transactions) const
      {
        foreach(const Transaction& transaction, this->transactions)
          if(transaction.type == Transaction::Type::buy)
            transactions.append(transaction);
      }

      virtual void getSellTransactions(QList<Transaction>& transactions) const
      {
        foreach(const Transaction& transaction, this->transactions)
          if(transaction.type == Transaction::Type::sell)
            transactions.append(transaction);
      }

      virtual void removeTransaction(quint64 id)
      {
        transactions.remove(id);
      }

      virtual void updateTransaction(quint64 id, const Transaction& transaction)
      {
        Transaction& destTransaction = transactions[id];
        destTransaction = transaction;
        destTransaction.id = id;
      }

    } simBroker(*this, 100., 0., fee);

    // create simulation agent
    Bot::Session* botSession = botFactory.createSession(simBroker);
    botSession->setParameters(i == iterations - 1 && iterations > 1 ? bestParameters : parameters);
    simBroker.session = botSession;
    if(!botSession)
    {
      quit(LogModel::Type::error, "Could not create bot session.");
      return;
    }

    // run simulation
    clearMarkers();
    if(!trades.isEmpty())
    {
      quint64 startTime = trades.front().time;
      TradeHandler tradeHandler;
      for(QList<DataProtocol::Trade>::ConstIterator i = trades.begin(), end = trades.end(); i != end; ++i)
      {
        const DataProtocol::Trade& trade = *i;
        tradeHandler.add(trade, 0);
        simBroker.update(trade);
        if(trade.time - startTime > 45 * 60 * 1000)
          botSession->handle(trade, tradeHandler.values);
      }
    }

    // done
    delete botSession;

    double balanceBase, balanceComm;
    double lastPrice = trades.isEmpty() ? 0 : trades.back().price;
    simBroker.cancelAllOrders();
    simBroker.getBalance(balanceBase, balanceComm);
    double rating = balanceBase + balanceComm * lastPrice * (1. - fee);
    if(rating > bestRating)
    {
      bestRating = rating;
      memcpy(bestParameters, parameters, sizeof(double) * parameterCount);
    }

    QString report =  QString("Completed simulation with %1 and %2 ~= %3 (best = %4).").arg(DataModel::formatPrice(balanceBase), balanceComm < 0. ? (QString("-") + DataModel::formatAmount(balanceComm)) : DataModel::formatAmount(balanceComm), 
      DataModel::formatPrice(rating), DataModel::formatPrice(bestRating));
    particleSwarm.setRating(-rating);

    logMessage(LogModel::Type::information, report);

    if(i == iterations - 1)
    {
      QList<Bot::Broker::Transaction> transactions;
      ((Bot::Broker*)&simBroker)->getBuyTransactions(transactions);
      double highestPrice = lastPrice;
      foreach(const Bot::Broker::Transaction& transaction, transactions)
      {
        balanceBase += transaction.price * (1. + fee) * transaction.amount;
        balanceComm -= transaction.amount;
        if(transaction.price > highestPrice)
          highestPrice = transaction.price;
      }
      double rating2 = balanceBase + balanceComm * highestPrice * (1. - fee);
      for(unsigned int i = 0; i < parameterCount; ++i)
        logMessage(LogModel::Type::information, QString("parameter%1=%2").arg(i).arg(bestParameters[i]));

      QString report =  QString("Or maybe even with %1 and %2 ~= %3.").arg(DataModel::formatPrice(balanceBase), balanceComm < 0. ? (QString("-") + DataModel::formatAmount(balanceComm)) : DataModel::formatAmount(balanceComm), 
        DataModel::formatPrice(rating2));

      logMessage(LogModel::Type::information, report);
    }
  }

  quit(LogModel::Type::information, "Finished optimization.");
}

void BotsWidget::Thread::clearMarkers()
{
  class ClearMarkersAction : public Action
  {
  public:
    virtual void execute(BotsWidget& widget)
    {
      PublicDataModel* publicDataModel = widget.dataModel.getPublicDataModel();
      if(publicDataModel)
        publicDataModel->graphModel.clearMarkers();
    }
  };
  widget.actionQueue.append(new ClearMarkersAction);
  QTimer::singleShot(0, &widget, SLOT(executeActions()));
}

void BotsWidget::Thread::addMarker(quint64 time, GraphModel::Marker marker)
{
  class AddMarkerAction : public Action
  {
  public:
    quint64 time;
    GraphModel::Marker marker;
    AddMarkerAction(quint64 time, GraphModel::Marker marker) : time(time), marker(marker) {}
    virtual void execute(BotsWidget& widget)
    {
      PublicDataModel* publicDataModel = widget.dataModel.getPublicDataModel();
      if(publicDataModel)
        publicDataModel->graphModel.addMarker(time, marker);
    }
  };
  widget.actionQueue.append(new AddMarkerAction(time, marker));
  QTimer::singleShot(0, &widget, SLOT(executeActions()));
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

void BotsWidget::updateToolBarButtons()
{
  //QList<QModelIndex> selectedRows = getSelectedRows();

  bool hasMarket = marketService.isReady();
  //bool canSimulate = getSelectedRows().size() > 0;

  optimizeAction->setEnabled(hasMarket /*&& canSimulate*/);
  simulateAction->setEnabled(hasMarket);
  activateAction->setEnabled(hasMarket);
}
