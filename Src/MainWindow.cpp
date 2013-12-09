
#include "stdafx.h"

MainWindow::MainWindow() : settings(QSettings::IniFormat, QSettings::UserScope, "MeGustaCoin", "MeGustaCoin"), market(0)
{
  orderWidget = new OrdersWidget(this, settings);
  connect(this, SIGNAL(marketChanged(Market*)), orderWidget, SLOT(setMarket(Market*)));

  setWindowIcon(QIcon(":/Icons/bitcoin_big.png"));
  updateWindowTitle();
  setDockNestingEnabled(true);
  setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
  //setCentralWidget(orderWidget);
  QDockWidget* ordersDockWidget = new QDockWidget(tr("Orders"), this);
  //ordersDockWidget->setFeatures(QDockWidget::DockWidgetVerticalTitleBar | QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
  ordersDockWidget->setObjectName("Orders");
  ordersDockWidget->setWidget(orderWidget);
  addDockWidget(Qt::TopDockWidgetArea, ordersDockWidget);
  QDockWidget* transactionsDockWidget = new QDockWidget(tr("Transactions"), this );
  //transactionsDockWidget->setFeatures(QDockWidget::DockWidgetVerticalTitleBar | QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
  transactionsDockWidget->setObjectName("Orders");
  //transactionsDockWidget->setWidget(transactionsWidget);
  addDockWidget(Qt::TopDockWidgetArea, transactionsDockWidget);
  tabifyDockWidget(ordersDockWidget, transactionsDockWidget);
  //setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);
  resize(600, 400);

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

  settings.setValue("Geometry", saveGeometry());
  settings.setValue("WindowState", saveState());

  emit marketChanged(0);
  delete market;
  market = 0;
  updateWindowTitle();
}

void MainWindow::refresh()
{
  if(!market)
    return;
  market->loadOrders();
  market->loadBalance();
  market->loadTicker();
}

void MainWindow::open(const QString& marketName, const QString& userName, const QString& key, const QString& secret)
{
  logout();

  // login
  this->marketName = marketName;
  this->userName = userName;
  if(marketName == "Bitstamp/USD")
  {
    market = new BitstampMarket(userName, key, secret);
  }
  if(!market)
    return;
  connect(market, SIGNAL(balanceUpdated()), this, SLOT(updateWindowTitle()));
  connect(market, SIGNAL(tickerUpdated()), this, SLOT(updateWindowTitle()));

  // update gui
  updateWindowTitle();
  emit marketChanged(market);

  // request data
  refresh();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
  logout();

  QMainWindow::closeEvent(event);
}

void MainWindow::updateWindowTitle()
{
  if(!market)
    setWindowTitle(tr("MeGustaCoin Trading Client"));
  else
  {
    QString title;
    const Market::Balance* balance = market->getBalance();
    if(balance)
      title.sprintf("%.02f %s, %.02f %s - ", balance->availableUsd + balance->reservedUsd, market->getMarketCurrency(), balance->availableBtc + balance->reservedBtc, market->getCoinCurrency());
    title += marketName;
    const Market::TickerData* tickerData = market->getTickerData();
    if(tickerData)
    {
      QString tickerInfo;
      tickerInfo.sprintf(" - %.02f, %.02f bid, %.02f ask", tickerData->lastTradePrice, tickerData->highestBuyOrder, tickerData->lowestSellOrder);
      title += tickerInfo;
    }
    setWindowTitle(title);
  }
}

void MainWindow::about()
{
  QMessageBox::about(this, "About", "MeGustaCoin - Bitcoin Trading Client<br><br>Donald Pillou, 2013");
}
