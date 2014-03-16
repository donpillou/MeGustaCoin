
#include "stdafx.h"

BotService::BotService(DataModel& dataModel) : dataModel(dataModel), thread(0){}

BotService::~BotService()
{
  stop();
}

void BotService::start(const QString& botName, bool simulation, const QString& marketName, const QString& userName, const QString& key, const QString& secret)
{
  if(thread)
    return;

  thread = new WorkerThread(*this, eventQueue, botName, simulation, marketName, userName, key, secret);
  thread->start();
}

void BotService::stop()
{
  if(!thread)
    return;

  //jobQueue.append(0); // cancel thread message
  thread->canceled = true;
  thread->interrupt();
  thread->wait();
  delete thread;
  thread = 0;

  handleEvents(); // better than qDeleteAll(eventQueue.getAll()); ;)
}

void BotService::handleEvents()
{
  for(;;)
  {
    Event* event = 0;
    if(!eventQueue.get(event, 0) || !event)
      break;
    event->handle(*this);
    delete event;
  }
}

void BotService::WorkerThread::interrupt()
{
  connection.interrupt();
}

void BotService::WorkerThread::process()
{
  if(canceled)
    return;

  addMessage(LogModel::Type::information, "Connecting to data service...");

  // create connection
  if(!connection.connect())
  {
    addMessage(LogModel::Type::error, QString("Could not connect to data service: %1").arg(connection.getLastError()));
    return;
  }
  addMessage(LogModel::Type::information, "Connected to data service.");

  // subscribe
  if(!connection.subscribe("Bitstamp/USD", 0))
    goto error;

  // loop
  for(;;)
  {
    if(canceled)
      return;
    if(!connection.process(*this))
      break;
  }

error:
  addMessage(LogModel::Type::error, QString("Lost connection to data service: %1").arg(connection.getLastError()));
}

void BotService::WorkerThread::run()
{
  // init
  Market* market = 0;
  Bot* bot = 0;

  if(marketName == "Bitstamp/USD")
    market = new BitstampMarket(userName, key, secret);
  if(!market)
    return;
  
  Market::Balance balance;
  while(!market->loadBalance(balance))
  {
    if(canceled)
      goto cleanup;
    sleep(10 * 1000);
  }

  if(botName == "BuyBot")
    bot = new BuyBot();
  if(!bot)
    return;
  
  if(simulation)
    broker = new SimBroker(*this, 100., 0., balance.fee);
  else
  {
    QSettings file(QSettings::IniFormat, QSettings::UserScope, "MeGustaCoin", "BotData");
    double balanceBase = file.value("balanceBase", balance.availableUsd).toDouble();
    double balanceComm = file.value("balanceComm", balance.availableBtc).toDouble();

    BotBroker* botBroker = new BotBroker(*this, *market, balanceBase, balanceComm, balance.fee);
    broker = botBroker;

    int transactionCount = file.beginReadArray("Transactions");
    Bot::Broker::Transaction transaction;
    for(int i = 0; i < transactionCount; ++i)
    {
      file.setArrayIndex(i);
      transaction.id = file.value("id").toULongLong();
      transaction.date = file.value("date").toULongLong();
      transaction.price = file.value("price").toDouble();
      transaction.amount = file.value("amount").toDouble();
      transaction.fee = file.value("fee").toDouble();
      switch((Bot::Broker::Transaction::Type)file.value("type").toUInt())
      {
      case Bot::Broker::Transaction::Type::buy:
        transaction.type = Bot::Broker::Transaction::Type::buy;
        botBroker->loadTransaction(transaction);
        break;
      case Bot::Broker::Transaction::Type::sell:
        transaction.type = Bot::Broker::Transaction::Type::sell;
        botBroker->loadTransaction(transaction);
        break;
      }
    }
    file.endArray();
  }
  session = bot->createSession(*broker);
  if(!session)
    goto cleanup;

  // loop
  while(!canceled)
  {
    process();
    if(canceled)
      return;
    sleep(10 * 1000);
  }

  // cleanup
cleanup:
  delete session;
  session = 0;
  delete broker;
  broker = 0;
  delete bot;
  delete market;
}

void BotService::WorkerThread::addMessage(LogModel::Type type, const QString& message)
{
  class LogMessageEvent : public Event
  {
  public:
    LogMessageEvent(LogModel::Type type, const QString& message) : type(type), message(message) {}
  private:
    LogModel::Type type;
    QString message;
  public: // Event
    virtual void handle(BotService& botService)
    {
        botService.dataModel.logModel.addMessage(type, message);
    }
  };
  eventQueue.append(new LogMessageEvent(type, message));
  QTimer::singleShot(0, &botService, SLOT(handleEvents()));
}

void BotService::WorkerThread::addMarker(quint64 time, GraphModel::Marker marker)
{
  class AddMarkerEvent : public Event
  {
  public:
    quint64 time;
    GraphModel::Marker marker;
    AddMarkerEvent(quint64 time, GraphModel::Marker marker) : time(time), marker(marker) {}
    virtual void handle(BotService& botService)
    {
      PublicDataModel* publicDataModel = botService.dataModel.getPublicDataModel();
      if(publicDataModel)
        publicDataModel->graphModel.addMarker(time, marker);
    }
  };
  eventQueue.append(new AddMarkerEvent(time, marker));
  QTimer::singleShot(0, &botService, SLOT(handleEvents()));
}

void BotService::WorkerThread::saveTransactionsFile(double balanceBase, double balanceComm, const QHash<quint64, Bot::Broker::Transaction>& transactions)
{
  class SaveTransactionEvent : public Event
  {
  public:
    SaveTransactionEvent(bool simulation, double balanceBase, double balanceComm, const QHash<quint64, Bot::Broker::Transaction>& transactions) :
      simulation(simulation), balanceBase(balanceBase), balanceComm(balanceComm), transactions(transactions) {}
  private:
    bool simulation;
    double balanceBase;
    double balanceComm;
    QHash<quint64, Bot::Broker::Transaction> transactions;

  public: // Event
    virtual void handle(BotService& botService)
    {
      QSettings file(QSettings::IniFormat, QSettings::UserScope, "MeGustaCoin", simulation ? "BotDataSimulation" : "BotData");
      file.clear();
      file.setValue("balanceBase", balanceBase);
      file.setValue("balanceComm", balanceComm);

      file.beginWriteArray("Transactions");
      int i = 0;
      foreach(const Bot::Broker::Transaction& transaction, transactions)
      {
        file.setArrayIndex(i++);
        file.setValue("id", transaction.id);
        file.setValue("date", transaction.date);
        file.setValue("price", transaction.price);
        file.setValue("amount", transaction.amount);
        file.setValue("fee", transaction.fee);
        file.setValue("type", (unsigned int)transaction.type);
      }
      file.endArray();
    }
  };

  eventQueue.append(new SaveTransactionEvent(simulation, balanceBase, balanceComm, transactions));
  QTimer::singleShot(0, &botService, SLOT(handleEvents()));}

void BotService::WorkerThread::addTransaction(const Bot::Broker::Transaction& transactionData)
{
  class AddTransactionEvent : public Event
  {
  public:
    AddTransactionEvent(const Market::Transaction& transaction) : transaction(transaction) {}
  private:
    Market::Transaction transaction;
  public: // Event
    virtual void handle(BotService& botService)
    {
      botService.dataModel.botTransactionModel.addTransaction(transaction);
    }
  };

  Market::Transaction transaction2;
  transaction2.id = QString::number(transactionData.id);
  transaction2.date = transactionData.date;
  transaction2.amount = transactionData.type == Bot::Broker::Transaction::Type::buy ? transactionData.amount : -transactionData.amount;
  transaction2.price = transactionData.price;
  transaction2.fee = transactionData.fee;
  transaction2.total = transaction2.amount * transactionData.price - transactionData.fee;

  eventQueue.append(new AddTransactionEvent(transaction2));
  QTimer::singleShot(0, &botService, SLOT(handleEvents()));
}

void BotService::WorkerThread::removeTransaction(const Bot::Broker::Transaction& transaction)
{
  class RemoveTransactionEvent : public Event
  {
  public:
    RemoveTransactionEvent(const QString& id) : id(id) {}
  private:
    QString id;
  public: // Event
    virtual void handle(BotService& botService)
    {
        botService.dataModel.botTransactionModel.removeTransaction(id);
    }
  };
  eventQueue.append(new RemoveTransactionEvent(QString::number(transaction.id)));
  QTimer::singleShot(0, &botService, SLOT(handleEvents()));
}

void BotService::WorkerThread::updateTransaction(const Bot::Broker::Transaction& transaction)
{
  addTransaction(transaction);
}

void BotService::WorkerThread::addOrder(const Market::Order& order)
{
  class AddOrderEvent : public Event
  {
  public:
    AddOrderEvent(const Market::Order& order) : order(order) {}
  private:
    Market::Order order;
  public: // Event
    virtual void handle(BotService& botService)
    {
        botService.dataModel.botOrderModel.addOrder(order);
    }
  };
  eventQueue.append(new AddOrderEvent(order));
  QTimer::singleShot(0, &botService, SLOT(handleEvents()));
}

void BotService::WorkerThread::removeOrder(const Market::Order& order)
{
  class RemoveOrderEvent : public Event
  {
  public:
    RemoveOrderEvent(const QString& id) : id(id) {}
  private:
    QString id;
  public: // Event
    virtual void handle(BotService& botService)
    {
        botService.dataModel.botOrderModel.removeOrder(id);
    }
  };
  eventQueue.append(new RemoveOrderEvent(order.id));
  QTimer::singleShot(0, &botService, SLOT(handleEvents()));
}

void BotService::WorkerThread::receivedTrade(quint64 channelId, const DataProtocol::Trade& trade)
{
  tradeHandler.add(trade, 0ULL);
  if(simulation)
  {
    if(startTime == 0)
      startTime = trade.time;
    if(trade.time - startTime <= 45 * 60 * 1000)
      return;
  }
  else if(trade.flags & DataProtocol::replayedFlag)
    return;
  broker->update(trade, *session);
  session->handle(trade, tradeHandler.values);
}

void BotService::BotBroker::update(const DataProtocol::Trade& trade, Bot::Session& session)
{
  time = trade.time / 1000ULL;

  foreach(const Order& order, openOrders)
    if (fabs(order.price - trade.price) <= 0.01)
    {
      refreshOrders(session);
      break;
    }

  cancelTimedOutOrders();
}

void BotService::BotBroker::refreshOrders(Bot::Session& session)
{
  QList<Market::Order> orders;
  if(!market.loadOrders(orders))
    return;
  QSet<QString> openOrderIds;
  foreach(const Market::Order& order, orders)
    openOrderIds.insert(order.id);
  for(QList<Order>::Iterator i = openOrders.begin(); i != openOrders.end();)
  {
    const Order& order = *i;
    if(!openOrderIds.contains(order.id))
    {
      Transaction transaction;
      transaction.id = nextTransactionId++;
      transaction.date = order.date;
      transaction.price  = order.price;
      transaction.amount = qAbs(order.amount);
      transaction.fee = order.fee;
      transaction.type = order.amount >= 0. ? Transaction::Type::buy : Transaction::Type::sell;
      transactions.insert(transaction.id, transaction);
      workerThread.addTransaction(transaction);

      if(order.amount >= 0.)
      {
        lastBuyTime = time;
        balanceComm += qAbs(order.amount);
      }
      else
      {
        lastSellTime = time;
        balanceBase += qAbs(order.amount) * order.price - order.fee;
      }
      workerThread.saveTransactionsFile(balanceBase, balanceComm, transactions);

      workerThread.removeOrder(order);
      openOrders.erase(i++);
      if(order.amount >= 0.)
        session.handleBuy(transaction);
      else
        session.handleSell(transaction);
    }
    else
      ++i;
  }
}

void BotService::BotBroker::cancelTimedOutOrders()
{
  for(QList<Order>::Iterator i = openOrders.begin(); i != openOrders.end();)
  {
    const Order& order = *i;
    if(time >= order.timeout)
    {
      if(market.cancelOrder(order.id))
      {
        if(order.amount >= 0.)
          balanceBase += qAbs(order.amount) * order.price + order.fee;
        else
          balanceComm += qAbs(order.amount);

        workerThread.removeOrder(order);
        openOrders.erase(i++);
        continue;
      }
    }
    ++i;
  }
}

bool BotService::BotBroker::buy(double price, double amount, quint64 timeout)
{
  double fee = ceil(amount * price * this->fee * 100.) / 100.;
  double charge = amount * price + fee;
  if(charge > balanceBase)
    return false;

  Market::Order order;
  if(!market.createOrder(amount, price, order))
    return false;
  workerThread.addOrder(order);
  openOrders.append(Order(order, time + timeout));
  balanceBase -= charge;
  return true;
}

bool BotService::BotBroker::sell(double price, double amount, quint64 timeout)
{
  if(amount > balanceComm)
    return false;

  Market::Order order;
  if(!market.createOrder(-amount, price, order))
    return false;
  workerThread.addOrder(order);
  openOrders.append(Order(order, time + timeout));
  balanceComm -= amount;
  return true;
}

unsigned int BotService::BotBroker::getOpenBuyOrderCount() const
{
  unsigned int openBuyOrders = 0;
  foreach(const Order& order, openOrders)
    if(order.amount >= 0.)
      ++openBuyOrders;
  return openBuyOrders;
}

unsigned int BotService::BotBroker::getOpenSellOrderCount() const
{
  unsigned int openSellOrders = 0;
  foreach(const Order& order, openOrders)
    if(order.amount < 0.)
      ++openSellOrders;
  return openSellOrders;
}

void BotService::BotBroker::getTransactions(QList<Transaction>& transactions) const
{
  foreach(const Transaction& transaction, this->transactions)
    transactions.append(transaction);
}

void BotService::BotBroker::getBuyTransactions(QList<Transaction>& transactions) const
{
  foreach(const Transaction& transaction, this->transactions)
    if(transaction.type == Transaction::Type::buy)
      transactions.append(transaction);
}

void BotService::BotBroker::getSellTransactions(QList<Transaction>& transactions) const
{
  foreach(const Transaction& transaction, this->transactions)
    if(transaction.type == Transaction::Type::sell)
      transactions.append(transaction);
}

void BotService::BotBroker::removeTransaction(quint64 id)
{
  QHash<quint64, Transaction>::Iterator it = transactions.find(id);
  if(it == transactions.end())
    return;
  const Transaction& transaction = it.value();
  workerThread.removeTransaction(transaction);
  transactions.erase(it);
  workerThread.saveTransactionsFile(balanceBase, balanceComm, transactions);
}

void BotService::BotBroker::updateTransaction(quint64 id, const Transaction& transaction)
{
  Transaction& destTransaction = transactions[id];
  destTransaction = transaction;
  destTransaction.id = id;
  workerThread.updateTransaction(destTransaction);
  workerThread.saveTransactionsFile(balanceBase, balanceComm, transactions);
}

void BotService::BotBroker::loadTransaction(const Transaction& transaction)
{
  transactions.insert(transaction.id, transaction);
  workerThread.addTransaction(transaction);
  if(transaction.id >= nextTransactionId)
    nextTransactionId = transaction.id + 1;
  // don not call savetransactionsFile here
}

void BotService::SimBroker::update(const DataProtocol::Trade& trade, Bot::Session& session)
{
  time = trade.time / 1000ULL;

  for(QList<Order>::Iterator i = openOrders.begin(); i != openOrders.end();)
  {
    const Order& order = *i;
    if(time >= order.timeout)
    {
      if(order.amount >= 0.)
        balanceBase += qAbs(order.amount) * order.price + order.fee;
      else
        balanceComm += qAbs(order.amount);

      workerThread.removeOrder(order);
      openOrders.erase(i++);
      continue;
    }
    else if((order.amount > 0. && trade.price < order.price) ||
            (order.amount < 0. && trade.price > order.price) )
    {
      Transaction transaction;
      transaction.id = nextTransactionId++;
      transaction.date = order.date;
      transaction.price  = order.price;
      transaction.amount = qAbs(order.amount);
      transaction.fee = order.fee;
      transaction.type = order.amount >= 0. ? Transaction::Type::buy : Transaction::Type::sell;
      transactions.insert(transaction.id, transaction);
      workerThread.addTransaction(transaction);

      if(order.amount >= 0.)
      {
        lastBuyTime = time;
        balanceComm += qAbs(order.amount);
      }
      else
      {
        lastSellTime = time;
        balanceBase += qAbs(order.amount) * order.price - order.fee;
      }
      workerThread.saveTransactionsFile(balanceBase, balanceComm, transactions);

      workerThread.removeOrder(order);
      openOrders.erase(i++);

      if(order.amount >= 0.)
      {
        workerThread.addMarker(time, GraphModel::Marker::buyMarker);
        session.handleBuy(transaction);
      }
      else
      {
        workerThread.addMarker(time, GraphModel::Marker::sellMarker);
        session.handleSell(transaction);
      }

      continue;
    }

    ++i;
  }
}

bool BotService::SimBroker::buy(double price, double amount, quint64 timeout)
{
  double fee = ceil(amount * price * this->fee * 100.) / 100.;
  double charge = amount * price + fee;
  if(charge > balanceBase)
    return false;

  Market::Order order;
  order.id = QString::number(nextOrderId++);
  order.date = time;
  order.amount = amount;
  order.price = price;
  order.fee = fee;
  order.total = order.amount * order.price - order.fee;

  workerThread.addOrder(order);
  workerThread.addMarker(time, GraphModel::Marker::buyAttemptMarker);
  openOrders.append(Order(order, time + timeout));
  balanceBase -= charge;
  return true;
}

bool BotService::SimBroker::sell(double price, double amount, quint64 timeout)
{
  if(amount > balanceComm)
    return false;

  Market::Order order;
  order.id = QString::number(nextOrderId++);
  order.date = time;
  order.amount = -amount;
  order.price = price;
  order.fee = ceil(amount * price * this->fee * 100.) / 100.;
  order.total = order.amount * order.price - order.fee;

  workerThread.addOrder(order);
  workerThread.addMarker(time, GraphModel::Marker::sellAttemptMarker);
  openOrders.append(Order(order, time + timeout));
  balanceComm -= amount;
  return true;
}

unsigned int BotService::SimBroker::getOpenBuyOrderCount() const
{
  unsigned int openBuyOrders = 0;
  foreach(const Order& order, openOrders)
    if(order.amount >= 0.)
      ++openBuyOrders;
  return openBuyOrders;
}

unsigned int BotService::SimBroker::getOpenSellOrderCount() const
{
  unsigned int openSellOrders = 0;
  foreach(const Order& order, openOrders)
    if(order.amount < 0.)
      ++openSellOrders;
  return openSellOrders;
}

void BotService::SimBroker::getTransactions(QList<Transaction>& transactions) const
{
  foreach(const Transaction& transaction, this->transactions)
    transactions.append(transaction);
}

void BotService::SimBroker::getBuyTransactions(QList<Transaction>& transactions) const
{
  foreach(const Transaction& transaction, this->transactions)
    if(transaction.type == Transaction::Type::buy)
      transactions.append(transaction);
}

void BotService::SimBroker::getSellTransactions(QList<Transaction>& transactions) const
{
  foreach(const Transaction& transaction, this->transactions)
    if(transaction.type == Transaction::Type::sell)
      transactions.append(transaction);
}

void BotService::SimBroker::removeTransaction(quint64 id)
{
  QHash<quint64, Transaction>::Iterator it = transactions.find(id);
  if(it == transactions.end())
    return;
  const Transaction& transaction = it.value();
  workerThread.removeTransaction(transaction);
  transactions.erase(it);
  workerThread.saveTransactionsFile(balanceBase, balanceComm, transactions);
}

void BotService::SimBroker::updateTransaction(quint64 id, const Transaction& transaction)
{
  Transaction& destTransaction = transactions[id];
  destTransaction = transaction;
  destTransaction.id = id;
  workerThread.updateTransaction(destTransaction);
  workerThread.saveTransactionsFile(balanceBase, balanceComm, transactions);
}
