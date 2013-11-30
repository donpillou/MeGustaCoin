
#pragma once

class BitstampMarket;

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
  };

public slots:
  void loadData(int request);

signals:
  void dataLoaded(int request, const QVariant& data);

private:
  const BitstampMarket& market;
  QDateTime lastRequestTime;

  void avoidSpamming();
};

class BitstampMarket : public Market
{
  Q_OBJECT

public:
  BitstampMarket(const QString& userName, const QString& key, const QString& secret);
  ~BitstampMarket();

  virtual void loadOrders();
  virtual void loadBalance();
  virtual void loadTicker();
  virtual void createOrder(const QString& id, bool sell, double amout, double price);

signals:
  void requestData(int request);

private:
  QThread thread;
  BitstampWorker* worker;

  QString userName;
  QString key;
  QString secret;

private slots:
  void handleData(int request, const QVariant& data);

  friend class BitstampWorker;
};
