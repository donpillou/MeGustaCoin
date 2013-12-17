
#pragma once

class BitstampWorker;

class BitstampMarket : public Market
{
  Q_OBJECT

public:
  BitstampMarket(DataModel& dataModel, const QString& userName, const QString& key, const QString& secret);
  ~BitstampMarket();

  virtual void loadOrders();
  virtual void loadBalance();
  virtual void loadTicker();
  virtual void loadTransactions();
  virtual void loadLiveTrades();
  virtual void loadOrderBook();
  virtual void enableLiveTradeUpdates(bool enable);
  virtual void enableOrderBookUpdates(bool enable);
  virtual void createOrder(const QString& draftId, double amount, double price);
  virtual void cancelOrder(const QString& id, double oldAmount, double oldPrice);
  virtual void updateOrder(const QString& id, double amount, double price, double oldAmount, double oldPrice);
  virtual double getMaxSellAmout() const;
  virtual double getMaxBuyAmout(double price, double canceledAmount, double canceledPrice) const;
  virtual double getOrderCharge(double amount, double price) const;
  virtual QString formatAmount(double amount) const;
  virtual QString formatPrice(double price) const;

signals:
  void requestData(int request, QVariant args);

private slots:
  void updateLiveTrades();
  void updateOrderBook();

private:
  QThread thread;
  BitstampWorker* worker;

  QString marketCurrency; // USD
  QString coinCurrency; // BTC

  QString userName;
  QString key;
  QString secret;

  QDateTime lastLiveTradesLoad;
  QDateTime lastOrderBookLoad;

  bool liveTradeUpdatesEnabled;
  bool orderBookUpdatesEnabled;
  bool liveTradeUpdateTimerStarted;
  bool orderBookUpdateTimerStarted;

  static const quint64 liveTradesUpdateRate = 1337 * 10;
  static const quint64 orderBookUpdateRate = 1000 * 30;

private slots:
  void handleData(int request, const QVariant& args, const QVariant& data);
  void handleError(int request, const QVariant& args, const QStringList& errors);

  friend class BitstampWorker;
};

class BitstampWorker : public QObject
{
  Q_OBJECT

public:
  BitstampWorker(const BitstampMarket& market);

  enum class Request
  {
    openOrders,
    balance,
    ticker,
    buy,
    sell,
    cancel,
    transactions,
    liveTrades,
    liveTradesUpdate,
    orderBook,
    orderBookUpdate,
  };

public slots:
  void loadData(int request, QVariant args);

signals:
  void dataLoaded(int request, const QVariant& args, const QVariant& data);
  void error(int request, const QVariant& args, const QStringList& errors);

private:
  const BitstampMarket& market;
  QDateTime lastRequestTime;
  quint64 lastNonce;
  QDateTime lastLiveTradeUpdateTime;

  void avoidSpamming();
};

