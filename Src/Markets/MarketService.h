
#pragma once

class MarketService : public QObject
{
  Q_OBJECT

public:
  MarketService(DataModel& dataModel);
  ~MarketService();

  void login(const QString& marketName, const QString& userName, const QString& key, const QString& secret);
  void logout();

  bool isReady() const {return thread != 0;}

  //const QString& getMarketName() const {return marketName;}

  void loadOrders();
  void loadBalance();
  void loadTicker();
  void loadTransactions();
  //void loadLiveTrades();
  //void loadOrderBook();
  //void enableLiveTradeUpdates(bool enable);
  //void enableOrderBookUpdates(bool enable);
  void createOrder(const QString& draftId, double amount, double price);
  void cancelOrder(const QString& id);
  void updateOrder(const QString& id, double amount, double price);
  //double getMaxSellAmout() const;
  //double getMaxBuyAmout(double price, double canceledAmount = 0., double canceledPrice = 0.) const;

private:
  DataModel& dataModel;
  QThread* thread;
  Market* market;

  class Job
  {
  public:
    enum class Type
    {
      loadTransactions,
      loadOrders,
      loadBalance,
      loadTickerData,
      //loadOrderBook,
      //loadTrades,
      createOrder,
      cancelOrder,
      updateOrder,
    };
    Type type;
    bool error;
    QString errorMessage;

    Job(Type type) : type(type), error(false) {}
    virtual ~Job() {}
  };

  class LoadTransactionsJob : public Job
  {
  public:
    QList<Market::Transaction> transactions;

    LoadTransactionsJob() : Job(Type::loadTransactions) {}
  };

  class LoadOrdersJob : public Job
  {
  public:
    QList<Market::Order> orders;

    LoadOrdersJob() : Job(Type::loadOrders) {}
  };

  class LoadBalanceJob : public Job
  {
  public:
    Market::Balance balance;

    LoadBalanceJob() : Job(Type::loadBalance) {}
  };

  class LoadTickerDataJob : public Job
  {
  public:
    Market::TickerData tickerData;

    LoadTickerDataJob() : Job(Type::loadTickerData) {}
  };
  
  /*class LoadOrderBookJob : public Job
  {
  public:
    quint64 date;
    QList<Market::OrderBookEntry> bids;
    QList<Market::OrderBookEntry> asks;

    LoadOrderBookJob() : Job(Type::loadOrderBook) {}
  };*/

  class CreateOrderJob : public Job
  {
  public:
    QString draftId;
    double amount;
    double price;
    Market::Order order;

    CreateOrderJob() : Job(Type::createOrder) {}
  };

  class UpdateOrderJob : public CreateOrderJob
  {
  public:
    UpdateOrderJob() {type = Type::updateOrder;}
  };

  class CancelOrderJob : public Job
  {
  public:
    QString id;

    CancelOrderJob() : Job(Type::cancelOrder) {}
  };

  /*class LoadTradesJob : public Job
  {
  public:
    QList<Market::Trade> trades;

    LoadTradesJob() : Job(Type::loadTrades) {}
  };*/

  JobQueue<Job*> queuedJobs;
  JobQueue<Job*> finishedJobs;

private slots:
  void finalizeJobs();

private:
  void processJobs();
  void processJob(Job* job);
};
