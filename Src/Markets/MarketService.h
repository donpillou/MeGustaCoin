
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

  void loadOrders();
  void loadBalance();
  void loadTransactions();
  QString createOrderDraft(bool sell, double price);
  void updateOrderDraft(const QString& draftId, double amount, double price);
  void createOrder(const QString& draftId, double amount, double price);
  void cancelOrder(const QString& id);
  void updateOrder(const QString& id, double amount, double price);

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
      createOrder,
      cancelOrder,
      updateOrder,
      updateOrderDraft,
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

  class UpdateOrderDraftJob : public Job
  {
  public:
    QString draftId;
    double amount;
    double price;
    Market::Order order;

    UpdateOrderDraftJob() : Job(Type::updateOrderDraft) {}
  };

  class CancelOrderJob : public Job
  {
  public:
    QString id;

    CancelOrderJob() : Job(Type::cancelOrder) {}
  };

  JobQueue<Job*> queuedJobs;
  JobQueue<Job*> finishedJobs;

private slots:
  void finalizeJobs();

private:
  void processJobs();
  void processJob(Job* job);
};
