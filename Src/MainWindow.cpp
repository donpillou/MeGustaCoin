
#include "stdafx.h"

MainWindow::MainWindow() : settings(QSettings::IniFormat, QSettings::UserScope, "MeGustaCoin", "MeGustaCoin"), market(0), 
liveTradeUpdatesEnabled(false), orderBookUpdatesEnabled(false), graphUpdatesEnabled(false)
{
  ordersWidget = new OrdersWidget(this, settings, dataModel);
  connect(this, SIGNAL(marketChanged(Market*)), ordersWidget, SLOT(setMarket(Market*)));
  transactionsWidget = new TransactionsWidget(this, settings, dataModel);
  connect(this, SIGNAL(marketChanged(Market*)), transactionsWidget, SLOT(setMarket(Market*)));
  tradesWidget = new TradesWidget(this, settings, dataModel);
  connect(this, SIGNAL(marketChanged(Market*)), tradesWidget, SLOT(setMarket(Market*)));
  bookWidget = new BookWidget(this, settings, dataModel);
  connect(this, SIGNAL(marketChanged(Market*)), bookWidget, SLOT(setMarket(Market*)));
  graphWidget = new GraphWidget(this, settings, dataModel);
  connect(this, SIGNAL(marketChanged(Market*)), graphWidget, SLOT(setMarket(Market*)));
  logWidget = new LogWidget(this, settings, dataModel.logModel);
  connect(this, SIGNAL(marketChanged(Market*)), logWidget, SLOT(setMarket(Market*)));

  setWindowIcon(QIcon(":/Icons/bitcoin_big.png"));
  updateWindowTitle();
  setDockNestingEnabled(true);
  setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
  //setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::East);
  resize(625, 400);

  QDockWidget* transactionsDockWidget = new QDockWidget(tr("Transactions"), this);
  transactionsDockWidget->setObjectName("Orders");
  transactionsDockWidget->setWidget(transactionsWidget);
  addDockWidget(Qt::TopDockWidgetArea, transactionsDockWidget);

  QDockWidget* ordersDockWidget = new QDockWidget(tr("Orders"), this);
  ordersDockWidget->setObjectName("Orders");
  ordersDockWidget->setWidget(ordersWidget);
  addDockWidget(Qt::TopDockWidgetArea, ordersDockWidget);
  tabifyDockWidget(transactionsDockWidget, ordersDockWidget);

  QDockWidget* graphDockWidget = new QDockWidget(tr("Live Graph"), this);
  connect(graphDockWidget, SIGNAL(visibilityChanged(bool)), this, SLOT(enableGraphUpdates(bool)));
  graphDockWidget->setObjectName("LiveGraph");
  graphDockWidget->setWidget(graphWidget);
  addDockWidget(Qt::TopDockWidgetArea, graphDockWidget);
  tabifyDockWidget(transactionsDockWidget, graphDockWidget);

  QDockWidget* bookDockWidget = new QDockWidget(tr("Order Book"), this);
  connect(bookDockWidget, SIGNAL(visibilityChanged(bool)), this, SLOT(enableOrderBookUpdates(bool)));
  bookDockWidget->setObjectName("OrderBook");
  bookDockWidget->setWidget(bookWidget);
  addDockWidget(Qt::TopDockWidgetArea, bookDockWidget, Qt::Vertical);
  bookDockWidget->hide();

  QDockWidget* logDockWidget = new QDockWidget(tr("Log"), this);
  logDockWidget->setObjectName("Log");
  logDockWidget->setWidget(logWidget);
  addDockWidget(Qt::TopDockWidgetArea, logDockWidget, Qt::Vertical);

  QDockWidget* tradesDockWidget = new QDockWidget(tr("Live Trades"), this);
  connect(tradesDockWidget, SIGNAL(visibilityChanged(bool)), this, SLOT(enableLiveTradesUpdates(bool)));
  tradesDockWidget->setObjectName("LiveTrades");
  tradesDockWidget->setWidget(tradesWidget);
  addDockWidget(Qt::TopDockWidgetArea, tradesDockWidget); //, Qt::Horizontal);
  tradesDockWidget->hide();

  QMenuBar* menuBar = this->menuBar();
  QMenu* menu = menuBar->addMenu(tr("&Market"));
  QAction* action = menu->addAction(tr("&Login..."));
  action->setShortcut(QKeySequence(QKeySequence::Open));
  connect(action, SIGNAL(triggered()), this, SLOT(login()));
  action = menu->addAction(tr("Log&out"));
  action->setShortcut(QKeySequence(QKeySequence::Close));
  connect(action, SIGNAL(triggered()), this, SLOT(logout()));
  menu->addSeparator();
  action = menu->addAction(tr("&Exit"));
  action->setShortcut(QKeySequence::Quit);
  connect(action, SIGNAL(triggered()), this, SLOT(close()));

  menu = menuBar->addMenu(tr("&View"));
  action = menu->addAction(tr("&Refresh"));
  action->setShortcut(QKeySequence(QKeySequence::Refresh));
  connect(action, SIGNAL(triggered()), this, SLOT(refresh()));
  menu->addSeparator();
  menu->addAction(ordersDockWidget->toggleViewAction());
  menu->addAction(transactionsDockWidget->toggleViewAction());
  menu->addAction(tradesDockWidget->toggleViewAction());
  menu->addAction(graphDockWidget->toggleViewAction());
  menu->addAction(bookDockWidget->toggleViewAction());
  menu->addAction(logDockWidget->toggleViewAction());

  menu = menuBar->addMenu(tr("&Help"));
  connect(menu->addAction(tr("&About...")), SIGNAL(triggered()), this, SLOT(about()));
  connect(menu->addAction(tr("About &Qt...")), SIGNAL(triggered()), qApp, SLOT(aboutQt()));

  restoreGeometry(settings.value("Geometry").toByteArray());
  restoreState(settings.value("WindowState").toByteArray());

  settings.beginGroup("Login");
  if(settings.value("Remember", 0).toUInt() >= 2)
  {
    QString market = settings.value("Market").toString();
    QString user = settings.value("User").toString();
    QString key = settings.value("Key").toString();
    QString secret = settings.value("Secret").toString();
    settings.endGroup();
    open(market, user, key, secret);
  }
  else
    settings.endGroup();

  if(!market)
    QTimer::singleShot(0, this, SLOT(login()));
}

MainWindow::~MainWindow()
{
  logout();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
  logout();

  settings.setValue("Geometry", saveGeometry());
  settings.setValue("WindowState", saveState());
  ordersWidget->saveState(settings);
  transactionsWidget->saveState(settings);
  tradesWidget->saveState(settings);
  bookWidget->saveState(settings);
  graphWidget->saveState(settings);
  logWidget->saveState(settings);

  QMainWindow::closeEvent(event);
}

void MainWindow::login()
{
  LoginDialog loginDialog(this, &settings);
  if(loginDialog.exec() != QDialog::Accepted)
    return;

  open(loginDialog.market(), loginDialog.userName(), loginDialog.key(), loginDialog.secret());
}

void MainWindow::logout()
{
  if(!market)
    return;

  delete market;
  market = 0;
  emit marketChanged(0);
  dataModel.orderModel.reset();
  dataModel.transactionModel.reset();
  dataModel.tradeModel.reset();
  dataModel.bookModel.reset();
  updateWindowTitle();

  dataModel.logModel.addMessage(LogModel::Type::information, QString("Closed %1").arg(marketName));
}

void MainWindow::refresh()
{
  ordersWidget->refresh();
  transactionsWidget->refresh();
}

void MainWindow::open(const QString& marketName, const QString& userName, const QString& key, const QString& secret)
{
  logout();

  // login
  this->marketName = marketName;
  this->userName = userName;
  if(marketName == "Bitstamp/USD")
  {
    market = new BitstampMarket(dataModel, userName, key, secret);
  }
  if(!market)
    return;

  dataModel.orderModel.setMarket(market);
  dataModel.transactionModel.setMarket(market);
  dataModel.tradeModel.setMarket(market);
  dataModel.bookModel.setMarket(market);

  connect(market, SIGNAL(balanceUpdated()), this, SLOT(updateWindowTitle()));
  connect(market, SIGNAL(tickerUpdated()), this, SLOT(updateWindowTitle()));
  dataModel.logModel.addMessage(LogModel::Type::information, QString(tr("Opened %1")).arg(marketName));

  // update gui
  updateWindowTitle();
  emit marketChanged(market);

  // request data
  refresh();
  market->loadLiveTrades();
  market->loadOrderBook();
  market->enableLiveTradeUpdates(liveTradeUpdatesEnabled || graphUpdatesEnabled);
  market->enableOrderBookUpdates(orderBookUpdatesEnabled /*|| graphUpdatesEnabled*/);
}

void MainWindow::updateWindowTitle()
{
  if(!market)
    setWindowTitle(tr("MeGustaCoin Market Client"));
  else
  {
    QString title;
    const Market::Balance* balance = market->getBalance();
    if(balance)
    {
      double usd = balance->availableUsd + balance->reservedUsd;
      double btc = balance->availableBtc + balance->reservedBtc;
      title = QString("%1 %2 / %3 %4 - ").arg(market->formatPrice(usd), market->getMarketCurrency(), market->formatAmount(btc), market->getCoinCurrency());
      //title = QString("%1 %2 / %3 %4 - ").arg(QLocale::system().toString(usd, 'f', 2), market->getMarketCurrency(), QLocale::system().toString(btc, 'f', 2), market->getCoinCurrency());
      //title.sprintf("%.02f %s, %.02f %s - ", usd, market->getMarketCurrency(), btc, market->getCoinCurrency());
    }
    title += marketName;
    const Market::TickerData* tickerData = market->getTickerData();
    if(tickerData)
    {
      title += QString(" - %1 / %2 bid / %3 ask").arg(market->formatPrice(tickerData->lastTradePrice), market->formatPrice(tickerData->highestBuyOrder), market->formatPrice(tickerData->lowestSellOrder));
      //QString tickerInfo;
      //tickerInfo.sprintf(" - %.02f, %.02f bid, %.02f ask", tickerData->lastTradePrice, tickerData->highestBuyOrder, tickerData->lowestSellOrder);
      //title += QString(" - %1 / %2 bid / %3 ask").arg(QLocale::system().toString(tickerData->lastTradePrice, 'f', 2), QLocale::system().toString(tickerData->highestBuyOrder, 'f', 2), QLocale::system().toString(tickerData->lowestSellOrder, 'f', 2));
    }
    setWindowTitle(title);
  }
}

void MainWindow::about()
{
  QMessageBox::about(this, "About", "MeGustaCoin - Bitcoin Market Client<br><a href=\"https://github.com/donpillou/MeGustaCoin\">https://github.com/donpillou/MeGustaCoin</a><br><br>Released under the GNU General Public License Version 3<br><br>MeGustaCoin uses the following third-party libraries and components:<br>&nbsp;&nbsp;- Qt (GUI)<br>&nbsp;&nbsp;- libcurl (HTTPS)<br>&nbsp;&nbsp;- LibQxt (JSON)<br>&nbsp;&nbsp;- <a href=\"http://www.famfamfam.com/lab/icons/silk/\">silk icons</a> (by Mark James)<br><br>-- Donald Pillou, 2013");
}

void MainWindow::enableLiveTradesUpdates(bool enable)
{
  liveTradeUpdatesEnabled = enable;
  if(!market)
    return;
  market->enableLiveTradeUpdates(liveTradeUpdatesEnabled || graphUpdatesEnabled);
}

void MainWindow::enableOrderBookUpdates(bool enable)
{
  orderBookUpdatesEnabled = enable;
  if(!market)
    return;
  market->enableOrderBookUpdates(orderBookUpdatesEnabled /*|| graphUpdatesEnabled */);
}

void MainWindow::enableGraphUpdates(bool enable)
{
  graphUpdatesEnabled = enable;
  if(!market)
    return;
  market->enableLiveTradeUpdates(liveTradeUpdatesEnabled || graphUpdatesEnabled);
  market->enableOrderBookUpdates(orderBookUpdatesEnabled /*|| graphUpdatesEnabled */);
}
