
#include "stdafx.h"

BitstampMarket::BitstampMarket(const QString& userName, const QString& key, const QString& secret) : userName(userName), key(key), secret(secret)
{
  marketCurrency = "USD";
  coinCurrency = "BTC";

  worker = new BitstampWorker(*this);
  worker->moveToThread(&thread);
  connect(this, SIGNAL(requestData(int)), worker, SLOT(loadData(int)), Qt::QueuedConnection);
  connect(worker, SIGNAL(dataLoaded(int, const QVariant&)), this, SLOT(handleData(int, const QVariant&)), Qt::BlockingQueuedConnection);
  thread.start();
}

BitstampMarket::~BitstampMarket()
{
  thread.quit();
  thread.wait();
  delete worker;
}

void BitstampMarket::loadOrders()
{
  emit requestData((int)BitstampWorker::Request::openOrders);
}

void BitstampMarket::loadBalance()
{
  emit requestData((int)BitstampWorker::Request::balance);
}

void BitstampMarket::loadTicker()
{
  emit requestData((int)BitstampWorker::Request::ticker);
}

void BitstampMarket::createOrder(const QString& id, bool sell, double amout, double price)
{
  // todo
}

void BitstampMarket::handleData(int request, const QVariant& data)
{
  switch(request)
  {
  case BitstampWorker::Request::balance:
    {
      QVariantMap balanceData = data.toMap();

      balance.reservedUsd = balanceData["usd_reserved"].toDouble();
      balance.reservedBtc = balanceData["btc_reserved"].toDouble();
      balance.availableUsd = balanceData["usd_available"].toDouble();
      balance.availableBtc = balanceData["btc_available"].toDouble();
      balance.fee =  balanceData["fee"].toDouble() * 0.01;

      emit balanceUpdated();
    }
    break;
  case BitstampWorker::Request::ticker:
    {
      QVariantMap tickerData = data.toMap();

      this->tickerData.lastTradePrice = tickerData["last"].toDouble();
      this->tickerData.highestBuyOrder = tickerData["bid"].toDouble();
      this->tickerData.lowestSellOrder = tickerData["ask"].toDouble();

      emit tickerUpdated();
    }
    break;
  case BitstampWorker::Request::openOrders:
    {
      QList<OrderModel::Order> orders;
      QVariantList ordersData = data.toList();
      foreach(const QVariant& orderDataVar, ordersData)
      {
        QVariantMap orderData = orderDataVar.toMap();
        orders.append(OrderModel::Order());
        OrderModel::Order& order = orders.back();
    
        order.id = orderData["id"].toString();
        QString type = orderData["type"].toString();
        if(type == "0")
          order.type = OrderModel::Order::Type::buy;
        if(type == "1")
          order.type = OrderModel::Order::Type::sell;
        order.date = orderData["datetime"].toString();
        order.price = orderData["price"].toDouble();
        order.amount = orderData["amount"].toDouble();
      }

      orderModel.setData(orders);
    }
    break;
  }
}

BitstampWorker::BitstampWorker(const BitstampMarket& market) : market(market) {}

void BitstampWorker::loadData(int request)
{
  avoidSpamming();

  const char* url = 0;
  bool isPublic = false;
  switch((Request)request)
  {
  case Request::openOrders:
    url = "https://www.bitstamp.net/api/open_orders/";
    break;
  case Request::balance:
    url = "https://www.bitstamp.net/api/balance/";
    break;
  case Request::ticker:
    url = "https://www.bitstamp.net/api/ticker/";
    isPublic = true;
    break;
  default:
    Q_ASSERT(false);
    return;
  }

  Download dl;
  char* dlData;
  if (isPublic)
  {
    if(!(dlData = dl.load(url)))
    {
      // todo
      return;
    }
  }
  else
  {
    QByteArray clientId(market.userName.toUtf8());
    QByteArray key(market.key.toUtf8());
    QByteArray secret(market.secret.toUtf8());

    QByteArray nonce(QString::number(QDateTime::currentDateTime().toTime_t()).toAscii());
    QByteArray message = nonce + clientId + key;
    QByteArray signature = Sha256::hmac(secret, message).toHex().toUpper();

    const char* fields[] = { "key", "signature", "nonce" };
    const char* values[] = { key.data(), signature.data(), nonce.data() };

    if(!(dlData = dl.loadPOST(url, fields, values, sizeof(fields) / sizeof(*fields))))
    {
      // todo
      return;
    }
  }

  QVariant orderData = QxtJSON::parse(dlData);
  dataLoaded(request, orderData);
  
  QVariantList fixStrangeQtBug(orderData.toList());
  orderData.clear(); 
}

void BitstampWorker::avoidSpamming()
{
  const qint64 queryDelay = 1337LL;
  QDateTime now = QDateTime::currentDateTime();
  qint64 elapsed = lastRequestTime.isNull() ? queryDelay : lastRequestTime.msecsTo(now);
  if(elapsed < queryDelay)
  {
    QMutex mutex;
    QWaitCondition condition;
    condition.wait(&mutex, queryDelay - elapsed);
    lastRequestTime = now;
    lastRequestTime.addMSecs(queryDelay - elapsed);
  }
  else
    lastRequestTime = now;
}
