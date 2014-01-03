
#include "stdafx.h"

MarketService::MarketService(DataModel& dataModel) : dataModel(dataModel), market(0), thread(0) {}

MarketService::~MarketService()
{
  logout();

  qDeleteAll(queuedJobs.getAll());
  qDeleteAll(finishedJobs.getAll());
}

void MarketService::login(const QString& marketName, const QString& userName, const QString& key, const QString& secret)
{
  if(market)
    return;

  // create market
  if(marketName == "Bitstamp/USD")
    market = new BitstampMarket(userName, key, secret);
  if(!market)
    return;

  class WorkerThread : public QThread
  {
  public:
    MarketService& service;
    WorkerThread(MarketService& service) : service(service) {}
    virtual void run() {service.processJobs();}
  };

  // create worker thread
  thread = new WorkerThread(*this);
  thread->start();

  dataModel.setMarket(marketName, market->getCoinCurrency(), market->getMarketCurrency());
  dataModel.logModel.addMessage(LogModel::Type::information, QString(tr("Opened %1")).arg(marketName));
}

void MarketService::logout()
{
  if(!thread)
    return;

  queuedJobs.prepend(0); // quit message
  thread->wait();
  delete thread;
  thread = 0;
  delete market;
  market = 0;

  qDeleteAll(queuedJobs.getAll());
  qDeleteAll(finishedJobs.getAll());

  dataModel.orderModel.reset();
  dataModel.transactionModel.reset();

  QString marketName = dataModel.getMarketName();
  dataModel.setMarket(QString(), QString(), QString());
  dataModel.logModel.addMessage(LogModel::Type::information, QString("Closed %1").arg(marketName));
}

void MarketService::loadTransactions()
{
  dataModel.transactionModel.setState(TransactionModel::State::loading);
  dataModel.logModel.addMessage(LogModel::Type::information, "Refreshing transactions...");
  queuedJobs.append(new LoadTransactionsJob());
}

void MarketService::loadOrders()
{
  dataModel.orderModel.setState(OrderModel::State::loading);
  dataModel.logModel.addMessage(LogModel::Type::information, "Refreshing orders...");
  queuedJobs.append(new LoadOrdersJob());
}

void MarketService::loadBalance()
{
  queuedJobs.append(new LoadBalanceJob());
}

void MarketService::loadTicker()
{
  queuedJobs.append(new LoadTickerDataJob());
}

void MarketService::createOrder(const QString& draftId, double amount, double price)
{
  const char* format = amount > 0. ? "Submitting buy order (%1 @ %2)..." : "Submitting sell order (%1 @ %2)...";
  dataModel.logModel.addMessage(LogModel::Type::information, QString(format).arg(dataModel.formatAmount(amount), dataModel.formatPrice(price)));
  dataModel.orderModel.setOrderState(draftId, OrderModel::Order::State::submitting);

  CreateOrderJob* job = new CreateOrderJob;
  job->draftId = draftId;
  job->amount = amount;
  job->price = price;
  queuedJobs.append(job);
}

void MarketService::updateOrder(const QString& id, double amount, double price)
{
  const OrderModel::Order* order = dataModel.orderModel.getOrder(id);
  if(!order)
    return;
  double oldAmount = order->type == OrderModel::Order::Type::buy ? order->amount : -order->amount;
  double oldPrice = order->price;

  const char* format = oldAmount > 0. ? "Canceling buy order (%1 @ %2)..." : "Canceling sell order (%1 @ %2)...";
  dataModel.logModel.addMessage(LogModel::Type::information, QString(format).arg(dataModel.formatAmount(oldAmount), dataModel.formatPrice(oldPrice)));
  dataModel.orderModel.setOrderState(id, OrderModel::Order::State::canceling);

  UpdateOrderJob* job = new UpdateOrderJob;
  job->draftId = id;
  job->amount = amount;
  job->price = price;
  queuedJobs.append(job);
}

void MarketService::cancelOrder(const QString& id)
{
  const OrderModel::Order* order = dataModel.orderModel.getOrder(id);
  if(!order)
    return;
  double oldAmount = order->type == OrderModel::Order::Type::buy ? order->amount : -order->amount;
  double oldPrice = order->price;

  const char* format = oldAmount > 0. ? "Canceling buy order (%1 @ %2)..." : "Canceling sell order (%1 @ %2)...";
  dataModel.logModel.addMessage(LogModel::Type::information, QString(format).arg(dataModel.formatAmount(oldAmount), dataModel.formatPrice(oldPrice)));
  dataModel.orderModel.setOrderState(id, OrderModel::Order::State::canceling);

  CancelOrderJob* job = new CancelOrderJob;
  job->id = id;
  queuedJobs.append(job);
}

void MarketService::processJobs()
{
  for(;;)
  {
    Job* job = 0;
    if(!queuedJobs.get(job) || !job)
      break;
    processJob(job);
    finishedJobs.append(job);
    QTimer::singleShot(0, this, SLOT(finalizeJobs()));
  }
}

void MarketService::processJob(Job* job)
{
  bool success = false;
  switch(job->type)
  {
  case Job::Type::loadTransactions:
    {
      LoadTransactionsJob* loadTransactionsJob = (LoadTransactionsJob*)job;
      success = market->loadTransactions(loadTransactionsJob->transactions);
    }
    break;
  case Job::Type::loadOrders:
    {
      LoadOrdersJob* loadOrdersJob = (LoadOrdersJob*)job;
      success = market->loadOrders(loadOrdersJob->orders);
    }
    break;
  case Job::Type::loadBalance:
    {
      LoadBalanceJob* loadBalanceJob = (LoadBalanceJob*)job;
      success = market->loadBalance(loadBalanceJob->balance);
    }
    break;
  case Job::Type::loadTickerData:
    {
      LoadTickerDataJob* loadTickerDataJob = (LoadTickerDataJob*)job;
      success = market->loadTicker(loadTickerDataJob->tickerData);
    }
    break;
  /*case Job::Type::loadOrderBook:
    {
      LoadOrderBookJob* loadOrderBookJob = (LoadOrderBookJob*)job;
      success = market->loadOrderBook(loadOrderBookJob->date, loadOrderBookJob->bids, loadOrderBookJob->asks);
    }
    break;
  case Job::Type::loadTrades:
    {
      LoadTradesJob* loadTradesJob = (LoadTradesJob*)job;
      success = market->loadTrades(loadTradesJob->trades);
    }
    break;*/
  case Job::Type::createOrder:
    {
      CreateOrderJob* createOrderJob = (CreateOrderJob*)job;
      success = market->createOrder(createOrderJob->amount, createOrderJob->price, createOrderJob->order);
    }
    break;
  case Job::Type::cancelOrder:
    {
      CancelOrderJob* cancelOrderJob = (CancelOrderJob*)job;
      success = market->cancelOrder(cancelOrderJob->id);
    }
    break;
  case Job::Type::updateOrder:
    {
      UpdateOrderJob* updateOrderJob = (UpdateOrderJob*)job;
      success = market->cancelOrder(updateOrderJob->draftId);
    }
    break;
  }

  if(!success)
  {
    job->error = true;
    job->errorMessage = market->getLastError();
  }
}

void MarketService::finalizeJobs()
{
  for(;;)
  {
    Job* job = 0;
    if(!finishedJobs.get(job, 0) || !job)
      break;
    if(!job->error)
    {
      switch(job->type)
      {
      case Job::Type::createOrder:
        {
          const CreateOrderJob* createOrderJob = (CreateOrderJob*)job;
          dataModel.orderModel.updateOrder(createOrderJob->draftId, createOrderJob->order);
          if(createOrderJob->amount > 0)
            dataModel.logModel.addMessage(LogModel::Type::information, tr("Submitted buy order"));
          else
            dataModel.logModel.addMessage(LogModel::Type::information, tr("Submitted sell order"));
        }
        break;
      case Job::Type::cancelOrder:
        {
          const CancelOrderJob* cancelOrderJob = (CancelOrderJob*)job;
          dataModel.orderModel.setOrderState(cancelOrderJob->id, OrderModel::Order::State::canceled);
          dataModel.logModel.addMessage(LogModel::Type::information, tr("Canceled order"));
        }
        break;
      case Job::Type::updateOrder:
        {
          const UpdateOrderJob* updateOrderJob = (UpdateOrderJob*)job;
          dataModel.orderModel.setOrderState(updateOrderJob->draftId, OrderModel::Order::State::submitting);
          dataModel.logModel.addMessage(LogModel::Type::information, tr("Canceled order"));

          createOrder(updateOrderJob->draftId, updateOrderJob->amount, updateOrderJob->price);
        }
        break;
      case Job::Type::loadBalance:
        {
          const LoadBalanceJob* loadBalanceJob = (LoadBalanceJob*)job;
          dataModel.setBalance(loadBalanceJob->balance);
          dataModel.logModel.addMessage(LogModel::Type::information, tr("Retrieved balance"));
        }
        break;
      case Job::Type::loadTickerData:
        {
          const LoadTickerDataJob* loadTickerDataJob = (LoadTickerDataJob*)job;
          dataModel.setTickerData(loadTickerDataJob->tickerData);
          dataModel.logModel.addMessage(LogModel::Type::information, tr("Retrieved ticker data"));
        }
        break;
      /*case Job::Type::loadOrderBook:
        {
          const LoadOrderBookJob* loadOrderBookJob = (LoadOrderBookJob*)job;
          const QList<Market::OrderBookEntry>& bids = loadOrderBookJob->bids;
          const QList<Market::OrderBookEntry>& asks = loadOrderBookJob->asks;

          dataModel.bookModel.setData(loadOrderBookJob->date, asks, bids);
          dataModel.logModel.addMessage(LogModel::Type::information, tr("Retrieved order book"));

          if(!bids.isEmpty() || !asks.isEmpty())
          {
            Market::TickerData tickerData = dataModel.getTickerData();
            if(!bids.isEmpty())
              tickerData.highestBuyOrder = bids.back().price;
            if(!asks.isEmpty())
              tickerData.lowestSellOrder = asks.back().price;
            dataModel.setTickerData(tickerData);
          }
        }
        break;
      case Job::Type::loadTrades:
        {
          const LoadTradesJob* loadTradesJob = (LoadTradesJob*)job;
          const QList<Market::Trade>& trades = loadTradesJob->trades;

          dataModel.tradeModel.addData(trades);
          dataModel.logModel.addMessage(LogModel::Type::information, tr("Retrieved live trades"));

          if(!trades.isEmpty())
          {
            Market::TickerData tickerData = dataModel.getTickerData();
            tickerData.lastTradePrice = trades.back().price;
            dataModel.setTickerData(tickerData);
          }
        }
        break;*/
      case Job::Type::loadOrders:
        {
          const LoadOrdersJob* loadOrdersJob = (LoadOrdersJob*)job;
          dataModel.orderModel.setData(loadOrdersJob->orders);
          dataModel.orderModel.setState(OrderModel::State::loaded);
          dataModel.logModel.addMessage(LogModel::Type::information, tr("Retrieved orders"));
        }
        break;
      case Job::Type::loadTransactions:
        {
          const LoadTransactionsJob* loadTransactionsJob = (LoadTransactionsJob*)job;
          dataModel.transactionModel.setData(loadTransactionsJob->transactions);
          dataModel.transactionModel.setState(TransactionModel::State::loaded);
          dataModel.logModel.addMessage(LogModel::Type::information, tr("Retrieved transactions"));
        }
        break;
      }
    }
    else
    {
      switch(job->type)
      {
      case Job::Type::loadOrders:
        dataModel.logModel.addMessage(LogModel::Type::error, tr("Could not load orders: %1").arg(job->errorMessage));
        dataModel.orderModel.setState(OrderModel::State::error);
        break;
      case Job::Type::loadBalance:
        dataModel.logModel.addMessage(LogModel::Type::error, tr("Could not load balance: %1").arg(job->errorMessage));
        break;
      case Job::Type::loadTickerData:
        dataModel.logModel.addMessage(LogModel::Type::error, tr("Could not load ticker data: %1").arg(job->errorMessage));
        break;
      /*case Job::Type::loadOrderBook:
        dataModel.logModel.addMessage(LogModel::Type::error, tr("Could not load order book: %1").arg(job->errorMessage));
        break;
      case Job::Type::loadTrades:
        dataModel.logModel.addMessage(LogModel::Type::error, tr("Could not load live trades: %1").arg(job->errorMessage));
        break;*/
      case Job::Type::createOrder:
        {
          const CreateOrderJob* createOrderJob = (CreateOrderJob*)job;
          if(createOrderJob->amount > 0.)
            dataModel.logModel.addMessage(LogModel::Type::error, tr("Could not submit buy order: %1").arg(job->errorMessage));
          else
            dataModel.logModel.addMessage(LogModel::Type::error, tr("Could not submit sell order: %1").arg(job->errorMessage));
          dataModel.orderModel.setOrderState(createOrderJob->draftId, OrderModel::Order::State::draft);
        }
        break;
      case Job::Type::cancelOrder:
        {
          const CancelOrderJob* cancelOrderJob = (CancelOrderJob*)job;
          dataModel.logModel.addMessage(LogModel::Type::error, tr("Could not cancel order: %1").arg(job->errorMessage));
          dataModel.orderModel.setOrderState(cancelOrderJob->id, OrderModel::Order::State::open);
        }
        break;
      case Job::Type::loadTransactions:
        dataModel.logModel.addMessage(LogModel::Type::error, tr("Could not load transactions: %1").arg(job->errorMessage));
        dataModel.transactionModel.setState(TransactionModel::State::error);
        break;
      }
    }

    delete job;
  }
}

