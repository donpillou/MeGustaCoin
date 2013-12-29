
#include "stdafx.h"

BitstampMarket::BitstampMarket(const QString& userName, const QString& key, const QString& secret) :
  marketCurrency(QObject::tr("USD")), coinCurrency(QObject::tr("BTC")),
  userName(userName), key(key), secret(secret),
  balanceLoaded(false), lastNonce(0) {}

bool BitstampMarket::loadBalanceAndFee()
{
  if(balanceLoaded)
    return true;
  Market::Balance balance;
  if(!loadBalance(balance))
    return false;
  return true;
}

bool BitstampMarket::createOrder(double amount, double price, Market::Order& order)
{
  if(!loadBalanceAndFee())
    return false;

  double maxAmount = fabs(amount > 0. ? getMaxBuyAmout(price) : getMaxSellAmout());
  if(fabs(amount) > maxAmount)
    amount = copysign(maxAmount, amount);

  QVariantMap args;
  args["amount"] = fabs(amount);
  args["price"] = price;

  const char* url = amount > 0. ? "https://www.bitstamp.net/api/buy/" : "https://www.bitstamp.net/api/sell/";
  VariantBugWorkaround result;
  if(!request(url, false, args, result))
    return false;

  QVariantMap orderData = result.toMap();

  order.id = orderData["id"].toString();
  QString type = orderData["type"].toString();
  if(type != "0" && type != "1")
  {
    error = "Received invalid response.";
    return false;
  }

  bool buy = type == "0";

  QString dateStr = orderData["datetime"].toString();
  int lastDot = dateStr.lastIndexOf('.');
  if(lastDot >= 0)
    dateStr.resize(lastDot);
  QDateTime date = QDateTime::fromString(dateStr, "yyyy-MM-dd hh:mm:ss");
  date.setTimeSpec(Qt::UTC);
  order.date = date.toTime_t();

  order.price = orderData["price"].toDouble();
  order.amount = fabs(orderData["amount"].toDouble());
  if(!buy)
    order.amount = -order.amount;
  order.total = getOrderCharge(buy ? order.amount : -order.amount, order.price);
  balanceLoaded = false;
  return true;
}

bool BitstampMarket::cancelOrder(const QString& id)
{
  QVariantMap args;
  args["id"] = id;
  VariantBugWorkaround result;
  if(!request("https://www.bitstamp.net/api/cancel_order/", false, args, result))
    return false;
  balanceLoaded = false;
  return true;
}

bool BitstampMarket::loadOrders(QList<Order>& orders)
{
  if(!loadBalanceAndFee())
    return false;

  VariantBugWorkaround result;
  if(!request("https://www.bitstamp.net/api/open_orders/", false, QVariantMap(), result))
    return false;

  QVariantList ordersData = result.toList();
  orders.reserve(ordersData.size());
  foreach(const QVariant& orderDataVar, ordersData)
  {
    QVariantMap orderData = orderDataVar.toMap();
    orders.append(Market::Order());
    Market::Order& order = orders.back();
    
    order.id = orderData["id"].toString();
    QString type = orderData["type"].toString();
    if(type != "0" && type != "1")
      continue;
    bool buy = type == "0";

    QString dateStr = orderData["datetime"].toString();
    QDateTime date = QDateTime::fromString(dateStr, "yyyy-MM-dd hh:mm:ss");
    date.setTimeSpec(Qt::UTC);
    order.date = date.toTime_t();

    order.price = orderData["price"].toDouble();
    order.amount = fabs(orderData["amount"].toDouble());
    if(!buy)
      order.amount = -order.amount;
    order.total = getOrderCharge(buy ? order.amount : -order.amount, order.price);
  }
  return true;
}

bool BitstampMarket::loadBalance(Balance& balance)
{
  VariantBugWorkaround result;
  if(!request("https://www.bitstamp.net/api/balance/", false, QVariantMap(), result))
    return false;

  QVariantMap balanceData = result.toMap();
  balance.reservedUsd = balanceData["usd_reserved"].toDouble();
  balance.reservedBtc = balanceData["btc_reserved"].toDouble();
  balance.availableUsd = balanceData["usd_available"].toDouble();
  balance.availableBtc = balanceData["btc_available"].toDouble();
  balance.fee =  balanceData["fee"].toDouble() * 0.01;
  this->balance = balance;
  this->balanceLoaded = true;
  return true;
}

bool BitstampMarket::loadTransactions(QList<Transaction>& transactions)
{
  VariantBugWorkaround result;
  if(!request("https://www.bitstamp.net/api/user_transactions/", false, QVariantMap(), result))
    return false;

  QVariantList transactionData = result.toList();
  transactions.reserve(transactionData.size());
  foreach(const QVariant& transactionDataVar, transactionData)
  {
    QVariantMap transactionData = transactionDataVar.toMap();
    transactions.append(Market::Transaction());
    Market::Transaction& transaction = transactions.back();
    
    transaction.id = transactionData["id"].toString();
    QString type = transactionData["type"].toString();
    if(type != "2")
      continue;

    QString dateStr = transactionData["datetime"].toString();
    QDateTime date = QDateTime::fromString(dateStr, "yyyy-MM-dd hh:mm:ss");
    date.setTimeSpec(Qt::UTC);
    transaction.date = date.toTime_t();
    transaction.fee = transactionData["fee"].toDouble();

    double value = transactionData["usd"].toDouble();
    bool buy = value < 0.;
    transaction.total = buy ? -(fabs(value) + transaction.fee) : (fabs(value) - transaction.fee);
    transaction.amount = fabs(transactionData["btc"].toDouble());
    if(!buy)
      transaction.amount = -transaction.amount;
    transaction.price = fabs(value) / fabs(transaction.amount);
  }

  return true;
}

bool BitstampMarket::loadTicker(TickerData& tickerData)
{
  VariantBugWorkaround result;
  if(!request("https://www.bitstamp.net/api/ticker/", true, QVariantMap(), result))
    return false;

  QVariantMap tickerDataVar = result.toMap();
  tickerData.lastTradePrice = tickerDataVar["last"].toDouble();
  tickerData.highestBuyOrder = tickerDataVar["bid"].toDouble();
  tickerData.lowestSellOrder = tickerDataVar["ask"].toDouble();
  return true;
}

bool BitstampMarket::loadTrades(QList<Trade>& trades)
{
  const char* url = "https://www.bitstamp.net/api/transactions/?time=minute";
  {
    QDateTime now = QDateTime::currentDateTime();
    qint64 elapsed = lastLiveTradeUpdateTime.isNull() ? 60 * 60 : lastLiveTradeUpdateTime.secsTo(now);
    if(elapsed > 60 - 10)
      url = "https://www.bitstamp.net/api/transactions/";
    lastLiveTradeUpdateTime = now;
  }

  VariantBugWorkaround result;
  if(!request(url, true, QVariantMap(), result))
    return false;

  QVariantList tradesData = result.toList();
  trades.reserve(tradesData.size());
  foreach(const QVariant& tradeDataVar, tradesData)
  {
    QVariantMap tradeData = tradeDataVar.toMap();

    trades.prepend(Market::Trade());
    Market::Trade& trade = trades.front();

    trade.id = tradeData["tid"].toString();
    trade.date = tradeData["date"].toULongLong();
    trade.price = tradeData["price"].toDouble();
    trade.amount = tradeData["amount"].toDouble();
  }

  return true;
}

bool BitstampMarket::loadOrderBook(quint64& date, QList<OrderBookEntry>& bids, QList<OrderBookEntry>& asks)
{
  VariantBugWorkaround result;
  if(!request("https://www.bitstamp.net/api/order_book/", true, QVariantMap(), result))
    return false;

  QVariantMap orderBookData = result.toMap();
  date = orderBookData["timestamp"].toULongLong();
  QVariantList askData = orderBookData["asks"].toList();
  QVariantList bidData = orderBookData["bids"].toList();
  asks.reserve(askData.size());
  QVariantList dataList;
  for(int i = askData.size() - 1; i >= 0; --i)
  {
    asks.append(Market::OrderBookEntry());
    Market::OrderBookEntry& item = asks.back();
    dataList = askData[i].toList();
    item.price = dataList[0].toDouble();
    item.amount = dataList[1].toDouble();
  }
  bids.reserve(bidData.size());
  for(int i = bidData.size() - 1; i >= 0; --i)
  {
    bids.append(Market::OrderBookEntry());
    Market::OrderBookEntry& item = bids.back();
    dataList = bidData[i].toList();
    item.price = dataList[0].toDouble();
    item.amount = dataList[1].toDouble();
  }
  return true;
}

double BitstampMarket::getMaxSellAmout() const
{
  return balance.availableBtc;
}

double BitstampMarket::getMaxBuyAmout(double price) const
{
  double fee = balance.fee; // e.g. 0.0044
  double availableUsd = balance.availableUsd;
  double additionalAvailableUsd = 0.; //floor(canceledAmount * canceledPrice * (1. + fee) * 100.) / 100.;
  double usdAmount = availableUsd + additionalAvailableUsd;
  double result = floor(((100. / ( 100. + (fee * 100.))) * usdAmount) * 100.) / 100.;
  result /= price;
  result = floor(result * 100000000.) / 100000000.;
  return result;
}

double BitstampMarket::getOrderCharge(double amount, double price) const
{
  if(amount < 0.) // sell order
    return floor(-amount * price / (1. + balance.fee) * 100.) / 100.;
  else // buy order
    return floor(amount * price * (1. + balance.fee) * -100.) / 100.;
}
/*
void BitstampMarket::handleData(int request, const QVariant& args, const QVariant& data)
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

      QString dateStr = orderData["datetime"].toString();
      int lastDot = dateStr.lastIndexOf('.');
      if(lastDot >= 0)
        dateStr.resize(lastDot);
      QDateTime date = QDateTime::fromString(dateStr, "yyyy-MM-dd hh:mm:ss");
      date.setTimeSpec(Qt::UTC);
      order.date = date.toLocalTime();

      order.price = orderData["price"].toDouble();
      order.amount = orderData["amount"].toDouble();
      order.total = getOrderCharge(order.type == OrderModel::Order::Type::buy ? order.amount : -order.amount, order.price);

      QString draftId = args.toMap()["draftid"].toString();
      dataModel.orderModel.updateOrder(draftId, order);
      if ((BitstampWorker::Request)request == BitstampWorker::Request::sell)
        dataModel.logModel.addMessage(LogModel::Type::information, tr("Submitted sell order"));
      else
        dataModel.logModel.addMessage(LogModel::Type::information, tr("Submitted buy order"));

      emit requestData((int)BitstampWorker::Request::balance, QVariant());
    }
    break;
  case BitstampWorker::Request::cancel:
    {
      QVariantMap cancelArgs = args.toMap();
      QString id = cancelArgs["id"].toString();
      bool updating = cancelArgs["updating"].toBool();
      dataModel.orderModel.setOrderState(id, updating ? OrderModel::Order::State::submitting : OrderModel::Order::State::canceled);
      dataModel.logModel.addMessage(LogModel::Type::information, tr("Canceled order"));

      if (updating)
      {
        bool sell = cancelArgs["sell"].toBool();
        double amount = cancelArgs["amount"].toDouble();
        double price = cancelArgs["price"].toDouble();

        const char* format = !sell ? "Submitting buy order (%1 @ %2)..." : "Submitting sell order (%1 @ %2)...";
        dataModel.logModel.addMessage(LogModel::Type::information, QString(format).arg(formatAmount(amount), formatPrice(price)));

        BitstampWorker::Request request = sell ? BitstampWorker::Request::sell : BitstampWorker::Request::buy;
        emit requestData((int)request, args);
      }
      else
        emit requestData((int)BitstampWorker::Request::balance, QVariant());
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
      dataModel.logModel.addMessage(LogModel::Type::information, tr("Retrieved balance"));
    }
    break;
  case BitstampWorker::Request::ticker:
    {
      QVariantMap tickerData = data.toMap();

      this->tickerData.lastTradePrice = tickerData["last"].toDouble();
      this->tickerData.highestBuyOrder = tickerData["bid"].toDouble();
      this->tickerData.lowestSellOrder = tickerData["ask"].toDouble();

      emit tickerUpdated();
      dataModel.logModel.addMessage(LogModel::Type::information, tr("Retrieved ticker data"));
    }
    break;
  case BitstampWorker::Request::orderBook:
  case BitstampWorker::Request::orderBookUpdate:
    {
      QVariantMap orderBookData = data.toMap();
      quint64 date = orderBookData["timestamp"].toULongLong();
      QVariantList askData = orderBookData["asks"].toList();
      QVariantList bidData = orderBookData["bids"].toList();
      QList<BookModel::Item> askItems;
      askItems.reserve(askData.size());
      QVariantList dataList;
      for(int i = askData.size() - 1; i >= 0; --i)
      {
        askItems.append(BookModel::Item());
        BookModel::Item& item = askItems.back();
        dataList = askData[i].toList();
        item.price = dataList[0].toDouble();
        item.amount = dataList[1].toDouble();
      }
      QList<BookModel::Item> bidItems;
      bidItems.reserve(bidData.size());
      for(int i = bidData.size() - 1; i >= 0; --i)
      {
        bidItems.append(BookModel::Item());
        BookModel::Item& item = bidItems.back();
        dataList = bidData[i].toList();
        item.price = dataList[0].toDouble();
        item.amount = dataList[1].toDouble();
      }
      dataModel.bookModel.setData(date, askItems, bidItems);
      if((BitstampWorker::Request)request == BitstampWorker::Request::orderBook)
        dataModel.logModel.addMessage(LogModel::Type::information, tr("Retrieved order book"));

      if(!bidItems.isEmpty())
        tickerData.highestBuyOrder = bidItems.back().price;
      if(!askItems.isEmpty())
        tickerData.lowestSellOrder = askItems.back().price;
      emit tickerUpdated();

      if(orderBookUpdatesEnabled && !orderBookUpdateTimerStarted)
      {
        orderBookUpdateTimerStarted = true;
        QTimer::singleShot(orderBookUpdateRate, this, SLOT(updateOrderBook()));
      }
    }
    break;
  case BitstampWorker::Request::liveTrades:
  case BitstampWorker::Request::liveTradesUpdate:
    {
      QList<TradeModel::Trade> liveTrades;
      QVariantList tradesData = data.toList();
      liveTrades.reserve(tradesData.size());
      foreach(const QVariant& tradeDataVar, tradesData)
      {
        QVariantMap tradeData = tradeDataVar.toMap();

        liveTrades.prepend(TradeModel::Trade());
        TradeModel::Trade& trade = liveTrades.front();

        trade.id = tradeData["tid"].toString();
        trade.date = tradeData["date"].toULongLong();
        trade.price = tradeData["price"].toDouble();
        trade.amount = tradeData["amount"].toDouble();
      }

      dataModel.tradeModel.addData(liveTrades);
      if((BitstampWorker::Request)request == BitstampWorker::Request::liveTrades)
        dataModel.logModel.addMessage(LogModel::Type::information, tr("Retrieved live trades"));

      if(!liveTrades.isEmpty())
      {
        tickerData.lastTradePrice = liveTrades.back().price;
        emit tickerUpdated();
      }

      if(liveTradeUpdatesEnabled && !liveTradeUpdateTimerStarted)
      {
        liveTradeUpdateTimerStarted = true;
        QTimer::singleShot(liveTradesUpdateRate, this, SLOT(updateLiveTrades()));
      }
    }
    break;
  case BitstampWorker::Request::openOrders:
    {
      QList<OrderModel::Order> orders;
      QVariantList ordersData = data.toList();
      orders.reserve(ordersData.size());
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

        QString dateStr = orderData["datetime"].toString();
        QDateTime date = QDateTime::fromString(dateStr, "yyyy-MM-dd hh:mm:ss");
        date.setTimeSpec(Qt::UTC);
        order.date = date.toLocalTime();

        order.price = orderData["price"].toDouble();
        order.amount = orderData["amount"].toDouble();
        order.total = getOrderCharge(order.type == OrderModel::Order::Type::buy ? order.amount : -order.amount, order.price);
      }

      dataModel.orderModel.setData(orders);
      dataModel.logModel.addMessage(LogModel::Type::information, tr("Retrieved orders"));
    }
    break;
  case BitstampWorker::Request::transactions:
    {
      QList<TransactionModel::Transaction> transactions;
      QVariantList transactionData = data.toList();
      transactions.reserve(transactionData.size());
      QString dateFormat = QLocale::system().dateTimeFormat(QLocale::ShortFormat);
      foreach(const QVariant& transactionDataVar, transactionData)
      {
        QVariantMap transactionData = transactionDataVar.toMap();
        transactions.append(TransactionModel::Transaction());
        TransactionModel::Transaction& transaction = transactions.back();
    
        transaction.id = transactionData["id"].toString();
        QString type = transactionData["type"].toString();
        if(type != "2")
          continue;

        QString dateStr = transactionData["datetime"].toString();
        QDateTime date = QDateTime::fromString(dateStr, "yyyy-MM-dd hh:mm:ss");
        date.setTimeSpec(Qt::UTC);
        transaction.date = date.toLocalTime();

        double value = transactionData["usd"].toDouble();
        transaction.type = value > 0. ? TransactionModel::Transaction::Type::sell : TransactionModel::Transaction::Type::buy;
        transaction.amount = fabs(transactionData["btc"].toDouble());
        transaction.price = fabs(value) / transaction.amount;
        transaction.fee = transactionData["fee"].toDouble();
        transaction.total = value > 0. ? (fabs(value) - transaction.fee) : -(fabs(value) + transaction.fee);
      }

    }
    break;
  }
}
*/
/*
void BitstampMarket::updateLiveTrades()
{
  liveTradeUpdateTimerStarted = false;
  emit requestData((int)BitstampWorker::Request::liveTradesUpdate, QVariant());
}

void BitstampMarket::updateOrderBook()
{
  orderBookUpdateTimerStarted = false;
  emit requestData((int)BitstampWorker::Request::orderBookUpdate, QVariant());
}
*/
bool BitstampMarket::request(const char* url, bool isPublic, const QVariantMap& params, QVariant& result)
{
  avoidSpamming();

  Download dl;
  char* dlData;
  if (isPublic)
  {
    if(!(dlData = dl.load(url)))
    {
      error = dl.getErrorString();
      return false;
    }
  }
  else
  {
    QByteArray clientId(this->userName.toUtf8());
    QByteArray key(this->key.toUtf8());
    QByteArray secret(this->secret.toUtf8());

    quint64 newNonce = QDateTime::currentDateTime().toTime_t();
    if(newNonce <= lastNonce)
      newNonce = lastNonce + 1;
    lastNonce = newNonce;

    QByteArray nonce(QString::number(newNonce).toAscii());
    QByteArray message = nonce + clientId + key;
    QByteArray signature = Sha256::hmac(secret, message).toHex().toUpper();
    QByteArray amount, price, id;

    const char* fields[10];
    const char* values[10];
    int i = 0;

    fields[i] = "key"; values[i++] = key.data();
    fields[i] = "signature"; values[i++] = signature.data();
    fields[i] = "nonce"; values[i++] = nonce.data();

    QList<QByteArray> buffers;
    for(QVariantMap::const_iterator j = params.begin(), end = params.end(); j != end; ++j, ++i)
    {
      Q_ASSERT(i < sizeof(fields) / sizeof(*fields));
      buffers.append(j.key().toUtf8());
      fields[i] = buffers.back().constData();
      buffers.append(j.value().toString().toUtf8());
      values[i] = buffers.back().constData();
    }

    if(!(dlData = dl.loadPOST(url, fields, values, i)))
    {
      error = dl.getErrorString();
      return false;
    }
  }

  if(strcmp(url, "https://www.bitstamp.net/api/cancel_order/") == 0) // does not return JSON
  {
    QString answer(dlData);
    QVariantMap cancelData;
    if(answer != "true")
      cancelData["error"] = answer;
    result = cancelData;
  }
  else
    result = QxtJSON::parse(dlData);

  if(result.toMap().contains("error"))
  {
    QStringList errors;
    struct ErrorStringCollector
    {
      static void collect(QVariant var, QStringList& errors)
      {
        switch(var.type())
        {
        case QVariant::String:
          errors.push_back(var.toString());
          break;
        case QVariant::List:
          {
            QVariantList list = var.toList();
            foreach(const QVariant& var, list)
              collect(var, errors);
          }
          break;
        case QVariant::Map:
          {
            QVariantMap map = var.toMap();
            for(QVariantMap::iterator i = map.begin(), end = map.end(); i != end; ++i)
              collect(i.value(), errors);
          }
          break;
        }
      }
    };
    ErrorStringCollector::collect(result, errors);
    error = errors.join(" ");
    return false;
  }
  else
    return true;
}

void BitstampMarket::avoidSpamming()
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
    lastRequestTime  = lastRequestTime.addMSecs(queryDelay - elapsed);
  }
  else
    lastRequestTime = now;
  // TODO: allow more than 1 request per second but limit requests to 600 per 10 minutes
}

BitstampMarket::VariantBugWorkaround::~VariantBugWorkaround()
{
  // Something is wrong with Qt. The QVariant destructor crashes when it tries to free a variant list.
  // So, lets prevent the destructor from doing so:
  struct VariantListDestructorBugfix
  {
    static void findLists(const QVariant& var, QList<QVariantList>& lists)
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
  VariantListDestructorBugfix::findLists(*this, lists);
  this->clear();
}
