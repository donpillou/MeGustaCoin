
#include "stdafx.h"

BotService::BotService(DataModel& dataModel) : dataModel(dataModel), thread(0){}

BotService::~BotService()
{
  stop();
}

void BotService::start(const QString& botName, const QString& marketName, const QString& userName, const QString& key, const QString& secret)
{
  if(thread)
    return;

  thread = new WorkerThread(*this, eventQueue, botName, marketName, userName, key, secret);
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
  
  botBroker = new BotBroker(*this, *market, balance.availableUsd, /*balance.availableBtc*/0., balance.fee);
  session = bot->createSession(*botBroker);
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
  delete botBroker;
  botBroker = 0;
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

void BotService::WorkerThread::receivedTrade(quint64 channelId, const DataProtocol::Trade& trade)
{
  tradeHandler.add(trade, 0ULL);
  if(trade.flags & DataProtocol::replayedFlag)
    return;
  botBroker->update(trade, *session);
  session->handle(trade, tradeHandler.values);
}

void BotService::BotBroker::update(const DataProtocol::Trade& trade, Bot::Session& session)
{
  time = QDateTime::currentDateTime().toTime_t();

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
      transaction.price  = order.price;
      transaction.amount = qAbs(order.amount);
      transaction.fee = order.fee;
      transaction.type = order.amount >= 0. ? Transaction::Type::buy : Transaction::Type::sell;
      transactions.insert(transaction.id, transaction);

      if(order.amount >= 0.)
        lastBuyTime = time;
      else
        lastSellTime = time;
      balanceComm += order.amount;

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
    if(order.timeout > time)
      openOrders.erase(i++);
    else
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
  openOrders.append(Order(order, time + timeout));
  balanceComm -= amount;
  return true;
}

double BotService::BotBroker::getBalanceBase() const
{
  return balanceBase;
}

double BotService::BotBroker::getBalanceComm() const
{
  return balanceComm;
}

double BotService::BotBroker::getFee() const
{
  return fee;
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

quint64 BotService::BotBroker::getTimeSinceLastBuy() const
{
  return time - lastBuyTime;
}

quint64 BotService::BotBroker::getTimeSinceLastSell() const
{
  return time - lastSellTime;
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
  transactions.remove(id);
}

void BotService::BotBroker::updateTransaction(quint64 id, const Transaction& transaction)
{
  Transaction& destTransaction = transactions[id];
  destTransaction = transaction;
  destTransaction.id = id;
}

void BotService::BotBroker::warning(const QString& message)
{
  workerThread.addMessage(LogModel::Type::warning, message);
}
