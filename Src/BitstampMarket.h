
#pragma once

class BitstampMarket;

class BitstampWorker : public QObject
{
  Q_OBJECT

public:
  BitstampWorker(const BitstampMarket& market);

public slots:
  void loadOrders();

signals:
  void ordersLoaded(const QVariant& data);

private:
  const BitstampMarket& market;
};

class BitstampMarket : private QObject, public Market
{
  Q_OBJECT

public:
  BitstampMarket(const QString& userName, const QString& key, const QString& secret);
  ~BitstampMarket();

  virtual void loadOrders();

private:
  QThread thread;
  BitstampWorker* worker;

  QString userName;
  QString key;
  QString secret;

private slots:
  void handleOrders(const QVariant& data);

  friend class BitstampWorker;
};
