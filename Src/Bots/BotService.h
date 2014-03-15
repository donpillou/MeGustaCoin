
#pragma once

class BotService : public QObject
{
  Q_OBJECT

public:
  BotService(DataModel& dataModel);
  ~BotService();

  void start(const QString& botName, const QString& marketName, const QString& userName, const QString& key, const QString& secret);
  void stop();

private:
  class Event
  {
  public:
    virtual ~Event() {}
    virtual void handle(BotService& botService) = 0;
  };

  class BotBroker;

  class WorkerThread : public QThread, public DataConnection::Callback
  {
  public:
    WorkerThread(BotService& botService, JobQueue<Event*>& eventQueue, const QString& botName, const QString& marketName, const QString& userName, const QString& key, const QString& secret) :
      botService(botService), eventQueue(eventQueue),
      botName(botName), marketName(marketName), userName(userName), key(key), secret(secret),
      botBroker(0), session(0), canceled(false) {}

    void interrupt();
    void addMessage(LogModel::Type type, const QString& message);

  public:
    BotService& botService;
    JobQueue<Event*>& eventQueue;
    QString botName;
    QString marketName;
    QString userName;
    QString key;
    QString secret;
    DataConnection connection;
    BotBroker* botBroker;
    Bot::Session* session;
    bool canceled;
    TradeHandler tradeHandler;

  private:
    void process();

  private: // QThread
    virtual void run();

  private: // DataConnection::Callback
    virtual void receivedChannelInfo(const QString& channelName) {}
    virtual void receivedSubscribeResponse(const QString& channelName, quint64 channelId) {}
    virtual void receivedUnsubscribeResponse(const QString& channelName, quint64 channelId) {}
    virtual void receivedTrade(quint64 channelId, const DataProtocol::Trade& trade);
    virtual void receivedTicker(quint64 channelId, const DataProtocol::Ticker& ticker) {}
    virtual void receivedErrorResponse(const QString& message) {}
  };

  class BotBroker : public Bot::Broker
  {
  public:
    BotBroker(WorkerThread& workerThread, Market& market, double balanceBase, double balanceComm, double fee) : workerThread(workerThread), market(market), balanceBase(balanceBase), balanceComm(balanceComm), fee(fee),
      time(0), lastBuyTime(0), lastSellTime(0), nextTransactionId(1) {}

    void update(const DataProtocol::Trade& trade, Bot::Session& session);

  private:
    class Order : public Market::Order
    {
    public:
      quint64 timeout;
      Order(const Market::Order& order, quint64 timeout) : Market::Order(order), timeout(timeout) {}
    };

  private:
    WorkerThread& workerThread;
    Market& market;
    QList<Order> openOrders;
    double balanceBase;
    double balanceComm;
    double fee;
    quint64 time;
    quint64 lastBuyTime;
    quint64 lastSellTime;
    QMap<quint64, Transaction> transactions;
    quint64 nextTransactionId;

  public:
    void refreshOrders(Bot::Session& session);
    void cancelTimedOutOrders();

  public: // Bot::Market
    virtual bool buy(double price, double amount, quint64 timeout);
    virtual bool sell(double price, double amount, quint64 timeout);
    virtual double getBalanceBase() const;
    virtual double getBalanceComm() const;
    virtual double getFee() const;
    virtual unsigned int getOpenBuyOrderCount() const;
    virtual unsigned int getOpenSellOrderCount() const;
    virtual quint64 getTimeSinceLastBuy() const;
    virtual quint64 getTimeSinceLastSell() const;

    virtual void getTransactions(QList<Transaction>& transactions) const;
    virtual void getBuyTransactions(QList<Transaction>& transactions) const;
    virtual void getSellTransactions(QList<Transaction>& transactions) const;
    virtual void removeTransaction(quint64 id);
    virtual void updateTransaction(quint64 id, const Transaction& transaction);

    virtual void warning(const QString& message);
  };

  DataModel& dataModel;
  WorkerThread* thread;
  JobQueue<Event*> eventQueue;

private slots:
  void handleEvents();
};
