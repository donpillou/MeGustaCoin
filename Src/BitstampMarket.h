
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
  virtual void createOrder(const QString& draftId, OrderType orderType, double amount, double price);
  virtual void cancelOrder(const QString& id, OrderType oldOrderType, double oldAmount, double oldPrice);
  virtual void updateOrder(const QString& id, OrderType orderType, double amount, double price, double oldAmount, double oldPrice);
  virtual double getMaxSellAmout() const;
  virtual double getMaxBuyAmout(double price, double canceledAmount, double canceledPrice) const;

signals:
  void requestData(int request, QVariant args);

private:
  QThread thread;
  BitstampWorker* worker;

  QString userName;
  QString key;
  QString secret;

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
  };

public slots:
  void loadData(int request, QVariant args);

signals:
  void dataLoaded(int request, const QVariant& args, const QVariant& data);
  void error(int request, const QVariant& args, const QStringList& errors);

private:
  const BitstampMarket& market;
  QDateTime lastRequestTime;

  void avoidSpamming();
};

