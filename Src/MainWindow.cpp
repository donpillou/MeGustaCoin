
#include "stdafx.h"

MainWindow::MainWindow() : settings(QSettings::IniFormat, QSettings::UserScope, "MeGustaCoin", "MeGustaCoin"),
  marketService(dataModel)
{
  connect(&dataModel, SIGNAL(changedMarket()), this, SLOT(updateFocusPublicDataModel()));
  connect(&dataModel, SIGNAL(changedBalance()), this, SLOT(updateWindowTitle()));
  //connect(&dataModel, SIGNAL(changedTickerData()), this, SLOT(updateWindowTitle()));

  publicDataModels.insert("MtGox/USD", new PublicDataModel(this, QColor(0x7f, 0x00, 0x7f, 0x70)));
  publicDataModels.insert("Bitstamp/USD", new PublicDataModel(this, QColor(0x00, 0x00, 0x7f, 0x70)));
  publicDataModels.insert("BtcChina/CNY", new PublicDataModel(this, QColor(0x00, 0x7f, 0x7f, 0x70)));
  publicDataModels.insert("Huobi/CNY", new PublicDataModel(this, QColor(0x61, 0x5f, 0x00, 0x70)));
  for(QMap<QString, PublicDataModel*>::iterator i = publicDataModels.begin(), end = publicDataModels.end(); i != end; ++i)
  {
    MarketData marketData;
    marketData.publicDataModel = i.value();
    connect(marketData.publicDataModel, SIGNAL(updatedTicker()), this, SLOT(updateWindowTitleTicker()));
    marketData.streamService = new MarketStreamService(this, dataModel, *marketData.publicDataModel, i.key());
    marketData.tradesWidget = new TradesWidget(this, settings, *marketData.publicDataModel);
    if(marketData.publicDataModel->getFeatures() & (int)MarketStream::Features::orderBook)
      marketData.bookWidget = new BookWidget(this, settings, *marketData.publicDataModel);
    else
      marketData.bookWidget = 0;
    marketData.graphWidget = new GraphWidget(this, settings, marketData.publicDataModel, i.key() + "_0", publicDataModels);
    marketDataList.append(marketData);
  }

  ordersWidget = new OrdersWidget(this, settings, dataModel, marketService, publicDataModels);
  transactionsWidget = new TransactionsWidget(this, settings, dataModel, marketService);
  //tradesWidget = new TradesWidget(this, settings, dataModel);
  //bookWidget = new BookWidget(this, settings, dataModel);
  graphWidget = new GraphWidget(this, settings, 0, QString(), publicDataModels);
  logWidget = new LogWidget(this, settings, dataModel.logModel);

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

  QDockWidget* graphDockWidget = new QDockWidget(this);
  graphDockWidget->setObjectName("LiveGraph");
  graphDockWidget->setWidget(graphWidget);
  graphWidget->updateTitle();
  addDockWidget(Qt::TopDockWidgetArea, graphDockWidget);
  tabifyDockWidget(transactionsDockWidget, graphDockWidget);

  for(QList<MarketData>::iterator i = marketDataList.begin(), end = marketDataList.end(); i != end; ++i)
  {
    MarketData& marketData = *i;
    marketData.graphDockWidget = new QDockWidget(this);
    connect(marketData.graphDockWidget, SIGNAL(visibilityChanged(bool)), this, SLOT(enableGraphUpdates(bool)));
    marketData.graphDockWidget->setObjectName(marketData.publicDataModel->getMarketName() + "LiveGraph");
    marketData.graphDockWidget->setWidget(marketData.graphWidget);
    marketData.graphWidget->updateTitle();
    addDockWidget(Qt::TopDockWidgetArea, marketData.graphDockWidget);
    //marketData.graphDockWidget->setFloating(true);
    marketData.graphDockWidget->hide();

    if(marketData.bookWidget)
    {
      marketData.bookDockWidget = new QDockWidget(this);
      connect(marketData.bookDockWidget, SIGNAL(visibilityChanged(bool)), this, SLOT(enableOrderBookUpdates(bool)));
      marketData.bookDockWidget->setObjectName(marketData.publicDataModel->getMarketName() + "OrderBook");
      marketData.bookDockWidget->setWidget(marketData.bookWidget);
      marketData.bookWidget->updateTitle();
      addDockWidget(Qt::TopDockWidgetArea, marketData.bookDockWidget);
      //marketData.bookDockWidget->setFloating(true);
      marketData.bookDockWidget->hide();
    }
    else
      marketData.bookDockWidget = 0;

    marketData.tradesDockWidget = new QDockWidget(this);
    connect(marketData.tradesDockWidget, SIGNAL(visibilityChanged(bool)), this, SLOT(enableLiveTradesUpdates(bool)));
    marketData.tradesDockWidget->setObjectName(marketData.publicDataModel->getMarketName() + "LiveTrades");
    marketData.tradesDockWidget->setWidget(marketData.tradesWidget);
    marketData.tradesWidget->updateTitle();
    addDockWidget(Qt::TopDockWidgetArea, marketData.tradesDockWidget);
    //marketData.tradesDockWidget->setFloating(true);
    marketData.tradesDockWidget->hide();
  }

  QDockWidget* logDockWidget = new QDockWidget(tr("Log"), this);
  logDockWidget->setObjectName("Log");
  logDockWidget->setWidget(logWidget);
  addDockWidget(Qt::TopDockWidgetArea, logDockWidget, Qt::Vertical);

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
  //menu->addAction(tradesDockWidget->toggleViewAction());
  menu->addAction(graphDockWidget->toggleViewAction());
  //menu->addAction(bookDockWidget->toggleViewAction());
  menu->addAction(logDockWidget->toggleViewAction());
  menu->addSeparator();
  for(QList<MarketData>::iterator i = marketDataList.begin(), end = marketDataList.end(); i != end; ++i)
  {
    MarketData& marketData = *i;
    QMenu* subMenu = menu->addMenu(marketData.publicDataModel->getMarketName());
    QAction* action = marketData.tradesDockWidget->toggleViewAction();
    subMenu->addAction(action);
    if(marketData.bookDockWidget)
    {
      action = marketData.bookDockWidget->toggleViewAction();
      subMenu->addAction(action);
    }
    action = marketData.graphDockWidget->toggleViewAction();
    subMenu->addAction(action);
  }

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

  if(!marketService.isReady())
    QTimer::singleShot(0, this, SLOT(login()));
}

MainWindow::~MainWindow()
{
  logout();

  for(QList<MarketData>::iterator i = marketDataList.begin(), end = marketDataList.end(); i != end; ++i)
  {
    MarketData& marketData = *i;

    delete marketData.tradesWidget;
    delete marketData.tradesDockWidget;
    delete marketData.bookWidget;
    delete marketData.bookDockWidget;
    delete marketData.graphWidget;
    delete marketData.graphDockWidget;

    delete marketData.streamService;
    delete marketData.publicDataModel;
  }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
  logout();

  settings.setValue("Geometry", saveGeometry());
  settings.setValue("WindowState", saveState());
  ordersWidget->saveState(settings);
  transactionsWidget->saveState(settings);
  graphWidget->saveState(settings);
  for(QList<MarketData>::iterator i = marketDataList.begin(), end = marketDataList.end(); i != end; ++i)
  {
    MarketData& marketData = *i;
    marketData.graphWidget->saveState(settings);
    marketData.tradesWidget->saveState(settings);
    if(marketData.bookWidget)
      marketData.bookWidget->saveState(settings);
  }
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
  marketService.logout();
}

void MainWindow::refresh()
{
  ordersWidget->refresh();
  transactionsWidget->refresh();
}

void MainWindow::open(const QString& marketName, const QString& userName, const QString& key, const QString& secret)
{
  logout();

  marketService.login(marketName, userName, key, secret);

  // update gui
  updateWindowTitle();

  // request data
  marketService.loadBalance();
  refresh();
//  marketService.loadTicker();
  /*
  market->loadLiveTrades();
  market->loadOrderBook();
  market->enableLiveTradeUpdates(liveTradeUpdatesEnabled || graphUpdatesEnabled);
  market->enableOrderBookUpdates(orderBookUpdatesEnabled || graphUpdatesEnabled);
  */
}

void MainWindow::updateWindowTitle()
{
  if(!marketService.isReady())
    setWindowTitle(tr("MeGustaCoin Market Client"));
  else
  {
    QString title;
    const Market::Balance& balance = dataModel.getBalance();
    double usd = balance.availableUsd + balance.reservedUsd;
    double btc = balance.availableBtc + balance.reservedBtc;
    if(usd != 0. || btc != 0. || balance.fee != 0.)
      title = QString("%1(%2) %3 / %4(%5) %6 - ").arg(dataModel.formatPrice(balance.availableUsd), dataModel.formatPrice(usd), dataModel.getMarketCurrency(), dataModel.formatAmount(balance.availableBtc), dataModel.formatAmount(btc), dataModel.getCoinCurrency());
    const QString& marketName = dataModel.getMarketName();
    title += marketName;
    if(!marketName.isEmpty())
    {
      const PublicDataModel* publicDataModel = publicDataModels[marketName];
      if(publicDataModel && !publicDataModel->graphModel.tickerSamples.isEmpty())
      {
        const GraphModel::TickerSample& tickerSample = publicDataModel->graphModel.tickerSamples.back();
        title += QString(" - %1 / %2 bid / %3 ask").arg(dataModel.formatPrice(tickerSample.last), dataModel.formatPrice(tickerSample.bid), dataModel.formatPrice(tickerSample.ask));
      }
    }
    setWindowTitle(title);
  }
}

void MainWindow::updateWindowTitleTicker()
{
  PublicDataModel* sourceModel = qobject_cast<PublicDataModel*>(sender());
  if(sourceModel->getMarketName() == dataModel.getMarketName())
    updateWindowTitle();
}

void MainWindow::updateFocusPublicDataModel()
{
  const QString& marketName = dataModel.getMarketName();
  PublicDataModel* publicDataModel = marketName.isEmpty() ? 0 : publicDataModels[marketName];
  graphWidget->setFocusPublicDataModel(publicDataModel);
  updateWindowTitle();
}

void MainWindow::about()
{
  QMessageBox::about(this, "About", "MeGustaCoin - Bitcoin Market Client<br><a href=\"https://github.com/donpillou/MeGustaCoin\">https://github.com/donpillou/MeGustaCoin</a><br><br>Released under the GNU General Public License Version 3<br><br>MeGustaCoin uses the following third-party libraries and components:<br>&nbsp;&nbsp;- Qt (GUI)<br>&nbsp;&nbsp;- libcurl (HTTP/HTTPS)<br>&nbsp;&nbsp;- easywsclient (Websockets)<br>&nbsp;&nbsp;- <a href=\"http://www.famfamfam.com/lab/icons/silk/\">silk icons</a> (by Mark James)<br><br>-- Donald Pillou, 2013");
}

void MainWindow::enableLiveTradesUpdates(bool enable)
{
  QObject* sender = this->sender();
  MarketData* marketData = 0;
  for(QList<MarketData>::iterator i = marketDataList.begin(), end = marketDataList.end(); i != end; ++i)
    if(i->tradesDockWidget == sender)
    {
      marketData = &*i;
      break;
    }
  Q_ASSERT(marketData);

  int enabledWidgets = marketData->enabledWidgets;
  if(enable)
    enabledWidgets |= (int)MarketData::EnabledWidgets::trades;
  else
    enabledWidgets &= ~(int)MarketData::EnabledWidgets::trades;

  if(enabledWidgets == marketData->enabledWidgets)
    return;
  marketData->enabledWidgets = enabledWidgets;
  if(enabledWidgets != 0)
    marketData->streamService->subscribe();
  else
    marketData->streamService->unsubscribe();
}

void MainWindow::enableOrderBookUpdates(bool enable)
{
  QObject* sender = this->sender();
  MarketData* marketData = 0;
  for(QList<MarketData>::iterator i = marketDataList.begin(), end = marketDataList.end(); i != end; ++i)
    if(i->bookDockWidget == sender)
    {
      marketData = &*i;
      break;
    }
  Q_ASSERT(marketData);

  int enabledWidgets = marketData->enabledWidgets;
  if(enable)
    enabledWidgets |= (int)MarketData::EnabledWidgets::book;
  else
    enabledWidgets &= ~(int)MarketData::EnabledWidgets::book;

  if(enabledWidgets == marketData->enabledWidgets)
    return;
  marketData->enabledWidgets = enabledWidgets;
  if(enabledWidgets != 0)
    marketData->streamService->subscribe();
  else
    marketData->streamService->unsubscribe();
}

void MainWindow::enableGraphUpdates(bool enable)
{
  QObject* sender = this->sender();
  MarketData* marketData = 0;
  for(QList<MarketData>::iterator i = marketDataList.begin(), end = marketDataList.end(); i != end; ++i)
    if(i->graphDockWidget == sender)
    {
      marketData = &*i;
      break;
    }
  Q_ASSERT(marketData);

  int enabledWidgets = marketData->enabledWidgets;
  if(enable)
    enabledWidgets |= (int)MarketData::EnabledWidgets::graph;
  else
    enabledWidgets &= ~(int)MarketData::EnabledWidgets::graph;

  if(enabledWidgets == marketData->enabledWidgets)
    return;
  marketData->enabledWidgets = enabledWidgets;
  if(enabledWidgets != 0)
    marketData->streamService->subscribe();
  else
    marketData->streamService->unsubscribe();
}
