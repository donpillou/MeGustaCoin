
#include "stdafx.h"

BitstampMarket::BitstampMarket(DataModel& dataModel, const QString& userName, const QString& key, const QString& secret) : Market(dataModel), userName(userName), key(key), secret(secret),
liveTradeUpdatesEnabled(false), liveTradeUpdateTimerStarted(false),
orderBookUpdatesEnabled(false), orderBookUpdateTimerStarted(false)
{
  marketCurrency = tr("USD");
  coinCurrency = tr("BTC");

  worker = new BitstampWorker(*this);
  worker->moveToThread(&thread);
  connect(this, SIGNAL(requestData(int, QVariant)), worker, SLOT(loadData(int, QVariant)), Qt::QueuedConnection);
  connect(worker, SIGNAL(dataLoaded(int, const QVariant&, const QVariant&)), this, SLOT(handleData(int, const QVariant&, const QVariant&)), Qt::BlockingQueuedConnection);
  connect(worker, SIGNAL(error(int, const QVariant&, const QStringList&)), this, SLOT(handleError(int, const QVariant&, const QStringList&)), Qt::BlockingQueuedConnection);
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

void BitstampMarket::loadLiveTrades()
{
  lastLiveTradesLoad = QDateTime::currentDateTime();
  emit requestData((int)BitstampWorker::Request::liveTrades, QVariant());
}

void BitstampMarket::loadOrderBook()
{
  lastOrderBookLoad = QDateTime::currentDateTime();
  emit requestData((int)BitstampWorker::Request::orderBook, QVariant());
}

void BitstampMarket::enableLiveTradeUpdates(bool enable)
{
  if(enable == liveTradeUpdatesEnabled)
    return;
  liveTradeUpdatesEnabled = enable;
  if(enable && !liveTradeUpdateTimerStarted)
  {
    liveTradeUpdateTimerStarted = true;
    qint64 timer = QDateTime::currentDateTime().msecsTo(lastLiveTradesLoad.addMSecs(liveTradesUpdateRate));
    QTimer::singleShot(qMax(0LL, timer), this, SLOT(updateLiveTrades()));
  }
}

void BitstampMarket::enableOrderBookUpdates(bool enable)
{
  if(enable == orderBookUpdatesEnabled)
    return;
  orderBookUpdatesEnabled = enable;
  if(enable && !orderBookUpdateTimerStarted)
  {
    orderBookUpdateTimerStarted = true;
    qint64 timer = QDateTime::currentDateTime().msecsTo(lastOrderBookLoad.addMSecs(orderBookUpdateRate));
    QTimer::singleShot(qMax(0LL, timer), this, SLOT(updateOrderBook()));
  }
}

void BitstampMarket::createOrder(const QString& draftId, double amount, double price)
{
  const char* format = amount > 0. ? "Submitting buy order (%1 @ %2)..." : "Submitting sell order (%1 @ %2)...";
  dataModel.logModel.addMessage(LogModel::Type::information, QString(format).arg(formatAmount(amount), formatPrice(price)));
  dataModel.orderModel.setOrderState(draftId, OrderModel::Order::State::submitting);

  QVariantMap args;
  args["draftid"] = draftId;
  args["amount"] = fabs(amount);
  args["price"] = price;
  BitstampWorker::Request request = amount > 0. ? BitstampWorker::Request::buy : BitstampWorker::Request::sell;
  emit requestData((int)request, args);
}

void BitstampMarket::cancelOrder(const QString& id, double oldAmount, double oldPrice)
{
  const char* format = oldAmount > 0. ? "Canceling buy order (%1 @ %2)..." : "Canceling sell order (%1 @ %2)...";
  dataModel.logModel.addMessage(LogModel::Type::information, QString(format).arg(formatAmount(oldAmount), formatPrice(oldPrice)));
  dataModel.orderModel.setOrderState(id, OrderModel::Order::State::canceling);

  QVariantMap args;
  args["id"] = id;
  emit requestData((int)BitstampWorker::Request::cancel, args);
}

void BitstampMarket::updateOrder(const QString& id, double amount, double price, double oldAmount, double oldPrice)
{
  const char* format = oldAmount > 0. ? "Canceling buy order (%1 @ %2)..." : "Canceling sell order (%1 @ %2)...";
  dataModel.logModel.addMessage(LogModel::Type::information, QString(format).arg(formatAmount(oldAmount), formatPrice(oldPrice)));
  dataModel.orderModel.setOrderState(id, OrderModel::Order::State::canceling);

  QVariantMap args;
  args["id"] = id;
  args["updating"] = true;
  args["draftid"] = id;
  args["amount"] = fabs(amount);
  args["price"] = price;
  args["sell"] = !(amount > 0.);
  emit requestData((int)BitstampWorker::Request::cancel, args);
}

double BitstampMarket::getMaxSellAmout() const
{
  return balance.availableBtc;
}

double BitstampMarket::getMaxBuyAmout(double price, double canceledAmount, double canceledPrice) const
{
  double fee = balance.fee; // e.g. 0.0044
  double availableUsd = balance.availableUsd;
  double additionalAvailableUsd = floor(canceledAmount * canceledPrice * (1. + fee) * 100.) / 100.;
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

QString BitstampMarket::formatAmount(double amount) const
{
  return QLocale::system().toString(fabs(amount), 'f', 8);
}

QString BitstampMarket::formatPrice(double price) const
{
  return QLocale::system().toString(price, 'f', 2);
}

const QString& BitstampMarket::getCoinCurrency() const
{
  return coinCurrency;
}

const QString& BitstampMarket::getMarketCurrency() const
{
  return marketCurrency;
}

void BitstampMarket::handleError(int request, const QVariant& args, const QStringList& errors)
{
  switch((BitstampWorker::Request)request)
  {
  case BitstampWorker::Request::openOrders:
    dataModel.logModel.addMessage(LogModel::Type::error, tr("Could not load orders:"));
    break;
  case BitstampWorker::Request::balance:
    dataModel.logModel.addMessage(LogModel::Type::error, tr("Could not load balance:"));
    break;
  case BitstampWorker::Request::ticker:
    dataModel.logModel.addMessage(LogModel::Type::error, tr("Could not load ticker data:"));
    break;
  case BitstampWorker::Request::orderBook:
  case BitstampWorker::Request::orderBookUpdate:
    dataModel.logModel.addMessage(LogModel::Type::error, tr("Could not load order book:"));
    if(orderBookUpdatesEnabled && !orderBookUpdateTimerStarted)
    {
      orderBookUpdateTimerStarted = true;
      QTimer::singleShot(orderBookUpdateRate, this, SLOT(updateOrderBook()));
    }
    break;
  case BitstampWorker::Request::liveTrades:
  case BitstampWorker::Request::liveTradesUpdate:
    dataModel.logModel.addMessage(LogModel::Type::error, tr("Could not load live trades:"));
    if(liveTradeUpdatesEnabled && !liveTradeUpdateTimerStarted)
    {
      liveTradeUpdateTimerStarted = true;
      QTimer::singleShot(liveTradesUpdateRate, this, SLOT(updateLiveTrades()));
    }
    break;
  case BitstampWorker::Request::buy:
  case BitstampWorker::Request::sell:
    {
      if((BitstampWorker::Request)request == BitstampWorker::Request::buy)
        dataModel.logModel.addMessage(LogModel::Type::error, tr("Could not submit buy order:"));
      else
        dataModel.logModel.addMessage(LogModel::Type::error, tr("Could not submit sell order:"));
      QString draftId = args.toMap()["draftid"].toString();
      dataModel.orderModel.setOrderState(draftId, OrderModel::Order::State::draft);
    }
    break;
  case BitstampWorker::Request::cancel:
    {
      dataModel.logModel.addMessage(LogModel::Type::error, tr("Could not cancel order:"));
      QString id = args.toMap()["id"].toString();
      dataModel.orderModel.setOrderState(id, OrderModel::Order::State::open);
    }
    break;
  case BitstampWorker::Request::transactions:
    dataModel.logModel.addMessage(LogModel::Type::error, tr("Could not load transactions:"));
    break;
  }
  foreach(const QString& error, errors)
    dataModel.logModel.addMessage(LogModel::Type::error, error);
}

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

      order.date = orderData["datetime"].toString();
      int lastDot = order.date.lastIndexOf('.');
      if(lastDot >= 0)
        order.date.resize(lastDot);
      QDateTime date = QDateTime::fromString(order.date, "yyyy-MM-dd hh:mm:ss");
      date.setTimeSpec(Qt::UTC);
      order.date = date.toLocalTime().toString(QLocale::system().dateTimeFormat(QLocale::ShortFormat));

      order.price = orderData["price"].toDouble();
      order.amount = orderData["amount"].toDouble();

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
      //else
        //dataModel.logModel.addMessage(LogModel::Type::information, tr("Updated order book")); // todo: remove this
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
      QString dateFormat = QLocale::system().dateTimeFormat(QLocale::ShortFormat);
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
        QDateTime date = QDateTime::fromString(order.date, "yyyy-MM-dd hh:mm:ss");
        date.setTimeSpec(Qt::UTC);
        order.date = date.toLocalTime().toString(dateFormat);

        order.price = orderData["price"].toDouble();
        order.amount = orderData["amount"].toDouble();
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

        transaction.date = transactionData["datetime"].toString();
        QDateTime date = QDateTime::fromString(transaction.date, "yyyy-MM-dd hh:mm:ss");
        date.setTimeSpec(Qt::UTC);
        transaction.date = date.toLocalTime().toString(dateFormat);

        double value = transactionData["usd"].toDouble();
        transaction.type = value > 0. ? TransactionModel::Transaction::Type::sell : TransactionModel::Transaction::Type::buy;
        transaction.amount = fabs(transactionData["btc"].toDouble());
        transaction.price = fabs(value) / transaction.amount;
        transaction.fee = transactionData["fee"].toDouble();
        transaction.balanceChange = value > 0. ? (fabs(value) - transaction.fee) : -(fabs(value) + transaction.fee);
      }

      dataModel.transactionModel.setData(transactions);
      dataModel.logModel.addMessage(LogModel::Type::information, tr("Retrieved transactions"));
    }
    break;
  }
}

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

BitstampWorker::BitstampWorker(const BitstampMarket& market) : market(market), lastNonce(QDateTime::currentDateTime().toTime_t()) {}

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
  case Request::liveTrades:
    url = "https://www.bitstamp.net/api/transactions/";
    isPublic = true;
    lastLiveTradeUpdateTime = QDateTime::currentDateTime();
    break;
  case Request::liveTradesUpdate:
    url = "https://www.bitstamp.net/api/transactions/?time=minute";
    isPublic = true;
    {
      QDateTime now = QDateTime::currentDateTime();
      qint64 elapsed = lastRequestTime.isNull() ? 60 * 60 : lastLiveTradeUpdateTime.secsTo(now);
      if(elapsed > 60 - 10)
        url = "https://www.bitstamp.net/api/transactions/";
      lastLiveTradeUpdateTime = now;
    }
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
  case Request::orderBook:
  case Request::orderBookUpdate:
    url = "https://www.bitstamp.net/api/order_book/";
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
      QStringList errors;
      errors.push_back(dl.getErrorString());
      emit error(request, params, errors);
      return;
    }
  }
  else
  {
    QByteArray clientId(market.userName.toUtf8());
    QByteArray key(market.key.toUtf8());
    QByteArray secret(market.secret.toUtf8());

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
      QStringList errors;
      errors.push_back(dl.getErrorString());
      emit error(request, params, errors);
      return;
    }
  }

  QVariant data;
  if((Request)request == Request::cancel)
  {
    QVariantMap cancelData;
    cancelData["success"] = QString(dlData) != "true";
    data = cancelData;
  }
  else
    data = QxtJSON::parse(dlData);

  QVariantMap dataMap = data.toMap();
  if(!dataMap["error"].isNull())
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
    ErrorStringCollector::collect(data, errors);
    emit error(request, params, errors);
  }
  else
    dataLoaded(request, params, data);

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
    lastRequestTime  = lastRequestTime.addMSecs(queryDelay - elapsed);
  }
  else
    lastRequestTime = now;
}
