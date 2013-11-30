
#include "stdafx.h"

BitstampMarket::BitstampMarket(const QString& userName, const QString& key, const QString& secret) : userName(userName), key(key), secret(secret)
{
  worker = new BitstampWorker(*this);
  worker->moveToThread(&thread);
  connect(worker, SIGNAL(ordersLoaded(const QVariant&)), this, SLOT(handleOrders(const QVariant&)), Qt::BlockingQueuedConnection);
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
  QTimer::singleShot(0, worker, SLOT(loadOrders()));
}

void BitstampMarket::handleOrders(const QVariant& orderData)
{
  QList<OrderModel::Order> orders;
  QVariantList ordersVar = orderData.toList();
  foreach(const QVariant& orderVar, ordersVar)
  {
    QVariantMap orderData = orderVar.toMap();
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

BitstampWorker::BitstampWorker(const BitstampMarket& market) : market(market) {}

void BitstampWorker::loadOrders()
{
  QByteArray clientId(market.userName.toUtf8());
  QByteArray key(market.key.toUtf8());
  QByteArray secret(market.secret.toUtf8());

  QByteArray nonce(QString::number(QDateTime::currentDateTime().toTime_t()).toAscii());
  QByteArray message = nonce + clientId + key;
  QByteArray signature = Sha256::hmac(secret, message).toHex().toUpper();

  const char* fields[] = { "key", "signature", "nonce" };
  const char* values[] = { key.data(), signature.data(), nonce.data() };

  Download dl;
  char* dlData;
  if(!(dlData = dl.loadPOST("https://www.bitstamp.net/api/open_orders/", fields, values, sizeof(fields) / sizeof(*fields))))
  {
    // todo
    return;
  }

  QVariant orderData = QxtJSON::parse(dlData);
  emit ordersLoaded(orderData);
  QVariantList fixStrangeQtBug(orderData.toList());
  orderData.clear(); 
}
