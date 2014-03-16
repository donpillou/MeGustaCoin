
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

  class Broker : public Bot::Broker
  {
  public:
    virtual void update(const DataProtocol::Trade& trade, Bot::Session& session) = 0;
  };

  class WorkerThread : public QThread, public DataConnection::Callback
  {
  public:
    WorkerThread(BotService& botService, JobQueue<Event*>& eventQueue, const QString& botName, const QString& marketName, const QString& userName, const QString& key, const QString& secret) :
      botService(botService), eventQueue(eventQueue),
      botName(botName), marketName(marketName), userName(userName), key(key), secret(secret),
      broker(0), session(0), canceled(false), forReal(false), startTime(0) {}

    void interrupt();
    void addMessage(LogModel::Type type, const QString& message);
    void addMarker(quint64 time, GraphModel::Marker marker);
    void addTransaction(const Bot::Broker::Transaction& transaction);
    void removeTransaction(const Bot::Broker::Transaction& transaction);
    void updateTransaction(const Bot::Broker::Transaction& transaction);
    void addOrder(const Market::Order& order);
    void removeOrder(const Market::Order& order);

  public:
    BotService& botService;
    JobQueue<Event*>& eventQueue;
    QString botName;
    QString marketName;
    QString userName;
    QString key;
    QString secret;
    DataConnection connection;
    Broker* broker;
    Bot::Session* session;
    bool canceled;
    TradeHandler tradeHandler;
    bool forReal;
    quint64 startTime;

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

  class BotBroker : public Broker
  {
  public:
    BotBroker(WorkerThread& workerThread, Market& market, double balanceBase, double balanceComm, double fee) : workerThread(workerThread), market(market), balanceBase(balanceBase), balanceComm(balanceComm), fee(fee),
      time(0), lastBuyTime(0), lastSellTime(0), nextTransactionId(1) {}

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

  private: // Broker
    virtual void update(const DataProtocol::Trade& trade, Bot::Session& session);

  private: // Bot::Market
    virtual bool buy(double price, double amount, quint64 timeout);
    virtual bool sell(double price, double amount, quint64 timeout);
    virtual double getBalanceBase() const {return balanceBase;}
    virtual double getBalanceComm() const {return balanceComm;}
    virtual double getFee() const {return fee;}
    virtual unsigned int getOpenBuyOrderCount() const;
    virtual unsigned int getOpenSellOrderCount() const;
    virtual quint64 getTimeSinceLastBuy() const{return time - lastBuyTime;}
    virtual quint64 getTimeSinceLastSell() const {return time - lastSellTime;}

    virtual void getTransactions(QList<Transaction>& transactions) const;
    virtual void getBuyTransactions(QList<Transaction>& transactions) const;
    virtual void getSellTransactions(QList<Transaction>& transactions) const;
    virtual void removeTransaction(quint64 id);
    virtual void updateTransaction(quint64 id, const Transaction& transaction);

    virtual void warning(const QString& message) {workerThread.addMessage(LogModel::Type::warning, message);}
  };

  class SimBroker : public Broker
  {
  public:
    SimBroker(WorkerThread& workerThread, double balanceBase, double balanceComm, double fee) : workerThread(workerThread), balanceBase(balanceBase), balanceComm(balanceComm), fee(fee),
      time(0), lastBuyTime(0), lastSellTime(0), nextOrderId(1), nextTransactionId(1) {}

  private:
    class Order : public Market::Order
    {
    public:
      quint64 timeout;
      Order(const Market::Order& order, quint64 timeout) : Market::Order(order), timeout(timeout) {}
    };

  private:
    WorkerThread& workerThread;
    QList<Order> openOrders;
    double balanceBase;
    double balanceComm;
    double fee;
    quint64 time;
    quint64 lastBuyTime;
    quint64 lastSellTime;
    QMap<quint64, Transaction> transactions;
    quint64 nextOrderId;
    quint64 nextTransactionId;

  private: // Broker
    virtual void update(const DataProtocol::Trade& trade, Bot::Session& session);

  private: // Bot::Market
    virtual bool buy(double price, double amount, quint64 timeout);
    virtual bool sell(double price, double amount, quint64 timeout);
    virtual double getBalanceBase() const {return balanceBase;}
    virtual double getBalanceComm() const {return balanceComm;}
    virtual double getFee() const {return fee;}
    virtual unsigned int getOpenBuyOrderCount() const;
    virtual unsigned int getOpenSellOrderCount() const;
    virtual quint64 getTimeSinceLastBuy() const{return time - lastBuyTime;}
    virtual quint64 getTimeSinceLastSell() const {return time - lastSellTime;}

    virtual void getTransactions(QList<Transaction>& transactions) const;
    virtual void getBuyTransactions(QList<Transaction>& transactions) const;
    virtual void getSellTransactions(QList<Transaction>& transactions) const;
    virtual void removeTransaction(quint64 id);
    virtual void updateTransaction(quint64 id, const Transaction& transaction);

    virtual void warning(const QString& message) {workerThread.addMessage(LogModel::Type::warning, message);}
  };


  DataModel& dataModel;
  WorkerThread* thread;
  JobQueue<Event*> eventQueue;

private slots:
  void handleEvents();
};
