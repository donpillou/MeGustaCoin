
#include "stdafx.h"

BitstampMarket::BitstampMarket(const QString& userName, const QString& key, const QString& secret) : userName(userName), key(key), secret(secret)
{
  marketCurrency = "USD";
  coinCurrency = "BTC";

  worker = new BitstampWorker(*this);
  worker->moveToThread(&thread);
  connect(this, SIGNAL(requestData(int, QVariant)), worker, SLOT(loadData(int, QVariant)), Qt::QueuedConnection);
  connect(worker, SIGNAL(dataLoaded(int, const QVariant&)), this, SLOT(handleData(int, const QVariant&)), Qt::BlockingQueuedConnection);
  thread.start();
}

BitstampMarket::~BitstampMarket()
{
  QEventLoop loop;
  connect(&thread, SIGNAL(finished()), &loop, SLOT(quit()));
  thread.quit();
  loop.exec();
  thread.wait();
  delete worker;
}

void BitstampMarket::loadOrders()
{
  emit requestData((int)BitstampWorker::Request::openOrders, QVariant());
}

void BitstampMarket::loadBalance()
{
  emit requestData((int)BitstampWorker::Request::balance, QVariant());
}

void BitstampMarket::loadTicker()
{
  emit requestData((int)BitstampWorker::Request::ticker, QVariant());
}

void BitstampMarket::loadTransactions()
{
  emit requestData((int)BitstampWorker::Request::transactions, QVariant());
}

void BitstampMarket::createOrder(const QString& draftId, bool sell, double amount, double price)
{
  orderModel.setOrderState(draftId, OrderModel::Order::State::submitting);
  QVariantMap args;
  args["draftid"] = draftId;
  args["amount"] = amount;
  args["price"] = price;
  emit requestData(sell ? (int)BitstampWorker::Request::sell : (int)BitstampWorker::Request::buy, args);
}

void BitstampMarket::cancelOrder(const QString& id)
{
  orderModel.setOrderState(id, OrderModel::Order::State::canceling);
  QVariantMap args;
  args["id"] = id;
  emit requestData((int)BitstampWorker::Request::cancel, args);
}

void BitstampMarket::updateOrder(const QString& id, bool sell, double amount, double price)
{
  orderModel.setOrderState(id, OrderModel::Order::State::canceling);
  {
    QVariantMap args;
    args["id"] = id;
    args["updating"] = true;
    emit requestData((int)BitstampWorker::Request::cancel, args);
  }
  {
    QVariantMap args;
    args["draftid"] = id;
    args["amount"] = amount;
    args["price"] = price;
    emit requestData(sell ? (int)BitstampWorker::Request::sell : (int)BitstampWorker::Request::buy, args);
  }
}

double BitstampMarket::getMaxSellAmout() const
{
  return balance.availableBtc;
}

double BitstampMarket::getMaxBuyAmout(double price) const
{
  double fee = balance.fee; // e.g. 0.0044
  double usdAmount = balance.availableUsd;
  double result = floor(((100. / ( 100. + (fee * 100.))) * usdAmount) * 100.) / 100.;
  result /= price;
  result = floor(result * 100000000.) / 100000000.;
  return result;
}

void BitstampMarket::handleData(int request, const QVariant& data)
{
  switch((BitstampWorker::Request)request)
  {
  case BitstampWorker::Request::sell:
  case BitstampWorker::Request::buy:
    {
      QVariantMap orderData = data.toMap();

      OrderModel::Order order;
      order.id = orderData["id"].toString();
      QString type = orderData["type"].toString();
      if(type == "0")
        order.type = OrderModel::Order::Type::buy;
      if(type == "1")
        order.type = OrderModel::Order::Type::sell;
      order.date = orderData["datetime"].toString();
      order.price = orderData["price"].toDouble();
      order.amount = orderData["amount"].toDouble();

      QString draftId = orderData["draftid"].toString();
      orderModel.updateOrder(draftId, order);
    }
    break;
  case BitstampWorker::Request::cancel:
    {
      QVariantMap cancelData = data.toMap();
      bool success = cancelData["success"].toBool();
      QString id = cancelData["id"].toString();
      bool updating = cancelData["updating"].toBool();
      orderModel.setOrderState(id, updating ? OrderModel::Order::State::submitting : OrderModel::Order::State::canceled);
    }
    break;
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
  case BitstampWorker::Request::transactions:
    {
      QList<TransactionModel::Transaction> transactions;
      QVariantList transactionData = data.toList();
      foreach(const QVariant& transactionDataVar, transactionData)
      {
        QVariantMap transactionData = transactionDataVar.toMap();
        transactions.append(TransactionModel::Transaction());
        TransactionModel::Transaction& transaction = transactions.back();
    
        transaction.id = transactionData["id"].toString();
        QString type = transactionData["type"].toString();
        if(type != "2")
          continue;
        transaction.date = transactionData["datetime"].toString();
        double value = transactionData["usd"].toDouble();
        transaction.type = value > 0. ? TransactionModel::Transaction::Type::sell : TransactionModel::Transaction::Type::buy;
        transaction.amount = fabs(transactionData["btc"].toDouble());
        transaction.price = fabs(value) / transaction.amount;
        transaction.fee = transactionData["fee"].toDouble();
        transaction.balanceChange = value > 0. ? (fabs(value) - transaction.fee) : -(fabs(value) + transaction.fee);
      }

      transactionModel.setData(transactions);
    }
    break;
  }
}

BitstampWorker::BitstampWorker(const BitstampMarket& market) : market(market) {}

void BitstampWorker::loadData(int request, QVariant params)
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
  case Request::buy:
    url = "https://www.bitstamp.net/api/buy/";
    break;
  case Request::sell:
    url = "https://www.bitstamp.net/api/sell/";
    break;
  case Request::cancel:
    url = "https://www.bitstamp.net/api/cancel_order/";
    break;
  case Request::transactions:
    url = "https://www.bitstamp.net/api/user_transactions/";
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
    QByteArray amount, price, id;

    const char* fields[10];
    const char* values[10];
    int i = 0;

    fields[i] = "key"; values[i++] = key.data();
    fields[i] = "signature"; values[i++] = signature.data();
    fields[i] = "nonce"; values[i++] = nonce.data();
    if((Request)request == Request::buy || (Request)request == Request::sell)
    {
      amount = params.toMap()["amount"].toString().toAscii();
      price = params.toMap()["price"].toString().toAscii();
      fields[i] = "amount"; values[i++] = amount.data();
      fields[i] = "price"; values[i++] = price.data();
    }
    if((Request)request == Request::cancel)
    {
      id = params.toMap()["id"].toString().toAscii();
      fields[i] = "id"; values[i++] = id.data();
    }

    if(!(dlData = dl.loadPOST(url, fields, values, i)))
    {
      // todo
      return;
    }
  }

  QVariant data = QxtJSON::parse(dlData);

  if((Request)request == Request::buy || (Request)request == Request::sell)
  {
    QString draftId = params.toMap()["draftid"].toString();
    QVariantMap orderData = data.toMap();
    orderData["draftid"] = draftId;
    dataLoaded(request, orderData);
  }
  else if((Request)request == Request::cancel)
  {
    QString id = params.toMap()["id"].toString();
    bool updating = params.toMap()["updating"].toBool();
    QVariantMap cancelData;
    cancelData["id"] = id;
    cancelData["success"] = QString(dlData) == "true";
    cancelData["updating"] = updating;
    dataLoaded(request, cancelData);
  }
  else
    dataLoaded(request, data);

  // something is wrong with qt, the QVariant destructor crashes when it tries to free a variant list.
  // so, lets prevent the destructor from doing so:
  struct VariantListDestructorBugfix
  {
    static void findLists(QVariant var, QList<QVariantList>& lists)
    {
      switch(var.type())
      {
      case QVariant::List:
        {
          QVariantList list = var.toList();
          lists.append(list);
          foreach(const QVariant& var, list)
            findLists(var, lists);
        }
        break;
      case QVariant::Map:
        {
          QVariantMap map = var.toMap();
          for(QVariantMap::iterator i = map.begin(), end = map.end(); i != end; ++i)
            findLists(i.value(), lists);
        }
        break;
      }
    }
  };

  QList<QVariantList> lists;
  VariantListDestructorBugfix::findLists(data, lists);
  data.clear();

  //QVariantList fixStrangeQtBug(data.toList());
  //data.clear(); 
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
    condition.wait(&mutex, queryDelay - elapsed); // wait without processing messages while waiting
    lastRequestTime = now;
    lastRequestTime.addMSecs(queryDelay - elapsed);
  }
  else
    lastRequestTime = now;
}
