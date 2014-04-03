
#include "stdafx.h"

MainWindow::MainWindow() : settings(QSettings::IniFormat, QSettings::UserScope, "Meguco", "MegucoClient"),
  marketService(dataModel), dataService(dataModel), botService(dataModel, botEntityManager)
{
  botEntityManager.delegateEntity(*new EBotService);
  botEntityManager.delegateEntity(*new EMarket);

  connect(&dataModel, SIGNAL(changedMarket()), this, SLOT(updateFocusPublicDataModel()));
  connect(&dataModel, SIGNAL(changedBalance()), this, SLOT(updateWindowTitle()));
  connect(&liveTradesSignalMapper, SIGNAL(mapped(const QString&)), this, SLOT(createLiveTradeWidget(const QString&)));
  connect(&liveGraphSignalMapper, SIGNAL(mapped(const QString&)), this, SLOT(createLiveGraphWidget(const QString&)));

  /*
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
  */

  ordersWidget = new OrdersWidget(this, settings, dataModel, marketService);
  transactionsWidget = new TransactionsWidget(this, settings, dataModel, marketService);
  graphWidget = new GraphWidget(this, settings, 0, QString(), dataModel.getDataChannels());
  botsWidget = new BotsWidget(this, settings, botEntityManager, botService);
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

  /*
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
  */
  QDockWidget* botsDockWidget = new QDockWidget(tr("Bots"), this);
  botsDockWidget->setObjectName("Bots");
  botsDockWidget->setWidget(botsWidget);
  addDockWidget(Qt::TopDockWidgetArea, botsDockWidget);
  tabifyDockWidget(transactionsDockWidget, botsDockWidget);

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

  viewMenu = menuBar->addMenu(tr("&View"));
  connect(viewMenu, SIGNAL(aboutToShow()), this, SLOT(updateViewMenu()));

  QMenu* toolsMenu = menuBar->addMenu(tr("&Tools"));
  connect(toolsMenu->addAction(tr("&Options...")), SIGNAL(triggered()), this, SLOT(showOptions()));

  menu = menuBar->addMenu(tr("&Help"));
  connect(menu->addAction(tr("&About...")), SIGNAL(triggered()), this, SLOT(about()));
  connect(menu->addAction(tr("About &Qt...")), SIGNAL(triggered()), qApp, SLOT(aboutQt()));

  QStringList liveTradesWidgets = settings.value("LiveTradesWidgets").toStringList();
  QStringList liveGraphWidgets = settings.value("LiveGraphWidgets").toStringList();
  foreach(const QString& channelName, liveTradesWidgets)
    createLiveTradeWidget(channelName);
  foreach(const QString& channelName, liveGraphWidgets)
    createLiveGraphWidget(channelName);

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

  startDataService();
  startBotService();
}

void MainWindow::startDataService()
{
  settings.beginGroup("DataServer");
  QString dataServer = settings.value("Address", "127.0.0.1:40123").toString();
  dataService.start(dataServer);
  settings.endGroup();
}

void MainWindow::startBotService()
{
  settings.beginGroup("BotServer");
  QString botServer = settings.value("Address", "127.0.0.1:40124").toString();
  QString botUser = settings.value("User").toString();
  QString botPassword = settings.value("Password").toString();
  botService.start(botServer, botUser, botPassword);
  settings.endGroup();
}

MainWindow::~MainWindow()
{
  logout();

  // manually delete widgets since they hold a reference to the data model
  for(QHash<QString, ChannelData>::Iterator i = channelDataMap.begin(), end = channelDataMap.end(); i != end; ++i)
  {
    ChannelData& channelData = i.value();
    delete channelData.tradesWidget;
    delete channelData.graphWidget;
  }
  delete ordersWidget;
  delete transactionsWidget;
  delete graphWidget;
  delete botsWidget;
  delete logWidget;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
  logout();

  settings.setValue("Geometry", saveGeometry());
  settings.setValue("WindowState", saveState());
  ordersWidget->saveState(settings);
  transactionsWidget->saveState(settings);
  graphWidget->saveState(settings);
  botsWidget->saveState(settings);

  QStringList openedLiveTradesWidgets;
  QStringList openedLiveGraphWidgets;
  for(QHash<QString, ChannelData>::iterator i = channelDataMap.begin(), end = channelDataMap.end(); i != end; ++i)
  {
    const QString& channelName = i.key();
    ChannelData& channelData = i.value();
    if(channelData.tradesWidget && qobject_cast<QDockWidget*>(channelData.tradesWidget->parent())->isVisible())
    {
      channelData.tradesWidget->saveState(settings);
      openedLiveTradesWidgets.append(channelName);
    }
    if(channelData.graphWidget && qobject_cast<QDockWidget*>(channelData.graphWidget->parent())->isVisible())
    {
      channelData.graphWidget->saveState(settings);
      openedLiveGraphWidgets.append(channelName);
    }
  }

  logWidget->saveState(settings);
  settings.setValue("LiveTradesWidgets", openedLiveTradesWidgets);
  settings.setValue("LiveGraphWidgets", openedLiveGraphWidgets);

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
  const QString& marketName = dataModel.getMarketName();
  if(!marketName.isEmpty())
    dataService.unsubscribe(marketName);
  marketService.logout();
}

void MainWindow::refresh()
{
  marketService.loadOrders();
  marketService.loadTransactions();
  marketService.loadBalance();
}

void MainWindow::open(const QString& marketName, const QString& userName, const QString& key, const QString& secret)
{
  logout();

  marketService.login(marketName, userName, key, secret);

  // update gui
  updateWindowTitle();

  // request data
  marketService.loadBalance();
  marketService.loadOrders();
  marketService.loadTransactions();
  dataService.subscribe(marketName);
}

void MainWindow::updateWindowTitle()
{
  if(!marketService.isReady())
    setWindowTitle(tr("Meguco Client"));
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
      const PublicDataModel& publicDataModel = dataModel.getDataChannel(marketName);
      double bid, ask;
      if(publicDataModel.getTicker(bid, ask))
        //title += QString(" - %1 / %2 bid / %3 ask").arg(dataModel.formatPrice(tickerSample.last), dataModel.formatPrice(bid), dataModel.formatPrice(ask));
        title += QString(" - %1 bid / %2 ask").arg(dataModel.formatPrice(bid), dataModel.formatPrice(ask));
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
  const PublicDataModel* oldFocusPublicDataModel = graphWidget->getFocusPublicDataModel();
  if(oldFocusPublicDataModel)
    disconnect(oldFocusPublicDataModel, SIGNAL(updatedTicker()), this, SLOT(updateWindowTitle()));
  const QString& marketName = dataModel.getMarketName();
  if(marketName.isEmpty())
  {
    graphWidget->setFocusPublicDataModel(0);
    updateWindowTitle();
  }
  else
  {
    PublicDataModel& publicDataModel = dataModel.getDataChannel(marketName);
    graphWidget->setFocusPublicDataModel(&publicDataModel);
    connect(&publicDataModel, SIGNAL(updatedTicker()), this, SLOT(updateWindowTitle()));
    updateWindowTitle();
  }
}

void MainWindow::updateViewMenu()
{
  viewMenu->clear();

  QAction* action = viewMenu->addAction(tr("&Refresh"));
  action->setShortcut(QKeySequence(QKeySequence::Refresh));
  connect(action, SIGNAL(triggered()), this, SLOT(refresh()));
  viewMenu->addSeparator();
  viewMenu->addAction(qobject_cast<QDockWidget*>(ordersWidget->parent())->toggleViewAction());
  viewMenu->addAction(qobject_cast<QDockWidget*>(transactionsWidget->parent())->toggleViewAction());
  viewMenu->addAction(qobject_cast<QDockWidget*>(graphWidget->parent())->toggleViewAction());
  viewMenu->addAction(qobject_cast<QDockWidget*>(logWidget->parent())->toggleViewAction());
  viewMenu->addAction(qobject_cast<QDockWidget*>(botsWidget->parent())->toggleViewAction());
  viewMenu->addSeparator();
  QMap<QString, PublicDataModel*> publicDataModels = dataModel.getDataChannels();
  for(QMap<QString, PublicDataModel*>::Iterator i = publicDataModels.begin(), end = publicDataModels.end(); i != end; ++i)
  {
    const QString& channelName = i.key();
    QMenu* subMenu = viewMenu->addMenu(channelName);

    ChannelData* channelData = 0;
    if(channelDataMap.contains(i.key()))
      channelData = &channelDataMap[channelName];

    if(channelData && channelData->tradesWidget)
      subMenu->addAction(qobject_cast<QDockWidget*>(channelData->tradesWidget->parent())->toggleViewAction());
    else
    {
      action = subMenu->addAction(tr("Live Trades"));
      liveTradesSignalMapper.setMapping(action, channelName);
      connect(action, SIGNAL(triggered()), &liveTradesSignalMapper, SLOT(map()));
    }
    if(channelData && channelData->graphWidget)
      subMenu->addAction(qobject_cast<QDockWidget*>(channelData->graphWidget->parent())->toggleViewAction());
    else
    {
      action = subMenu->addAction(tr("Live Graph"));
      liveGraphSignalMapper.setMapping(action, channelName);
      connect(action, SIGNAL(triggered()), &liveGraphSignalMapper, SLOT(map()));
    }
  }
}

void MainWindow::createLiveTradeWidget(const QString& channelName)
{
  ChannelData& channelData = channelDataMap[channelName];
  if(channelData.tradesWidget)
    return;

  PublicDataModel& publicDataModel = dataModel.getDataChannel(channelName);
  channelData.tradesWidget = new TradesWidget(this, settings, publicDataModel);

  QDockWidget* tradesDockWidget = new QDockWidget(this);
  //connect(tradesDockWidget->toggleViewAction(), SIGNAL(toggled(bool)), this, SLOT(enableTradesUpdates(bool)));
  connect(tradesDockWidget, SIGNAL(visibilityChanged(bool)), this, SLOT(enableTradesUpdates(bool)));
  tradesDockWidget->setObjectName(channelName + "LiveTrades");
  tradesDockWidget->setWidget(channelData.tradesWidget);
  channelData.tradesWidget->updateTitle();
  addDockWidget(Qt::TopDockWidgetArea, tradesDockWidget);
  
  dataService.subscribe(channelName);
}

void MainWindow::createLiveGraphWidget(const QString& channelName)
{
  ChannelData& channelData = channelDataMap[channelName];
  if(channelData.graphWidget)
    return;

  PublicDataModel& publicDataModel = dataModel.getDataChannel(channelName);
  channelData.graphWidget = new GraphWidget(this, settings, &publicDataModel, channelName + "_0",dataModel.getDataChannels());

  QDockWidget* graphDockWidget = new QDockWidget(this);
  //connect(graphDockWidget->toggleViewAction(), SIGNAL(toggled(bool)), this, SLOT(enableGraphUpdates(bool)));
  connect(graphDockWidget, SIGNAL(visibilityChanged(bool)), this, SLOT(enableGraphUpdates(bool)));
  graphDockWidget->setObjectName(channelName + "LiveGraph");
  graphDockWidget->setWidget(channelData.graphWidget);
  channelData.graphWidget->updateTitle();
  addDockWidget(Qt::TopDockWidgetArea, graphDockWidget);

  dataService.subscribe(channelName);
}

void MainWindow::showOptions()
{
  OptionsDialog optionsDialog(this, &settings);
  if(optionsDialog.exec() != QDialog::Accepted)
    return;

  startDataService();
  startBotService();
}

void MainWindow::about()
{
  QMessageBox::about(this, "About", "Meguco Client - A client for the Meguco Trade Framework<br><a href=\"https://github.com/donpillou/MegucoClient\">https://github.com/donpillou/MegucoClient</a><br><br>Released under the GNU General Public License Version 3<br><br>MeGustaCoin uses the following third-party libraries and components:<br>&nbsp;&nbsp;- Qt (GUI)<br>&nbsp;&nbsp;- libcurl (HTTP/HTTPS)<br>&nbsp;&nbsp;- easywsclient (Websockets)<br>&nbsp;&nbsp;- <a href=\"http://www.famfamfam.com/lab/icons/silk/\">silk icons</a> (by Mark James)<br><br>-- Donald Pillou, 2013-2014");
}

void MainWindow::enableTradesUpdates(bool enable)
{
  QDockWidget* sender = qobject_cast<QDockWidget*>(this->sender());
  QHash<QString, ChannelData>::Iterator it = channelDataMap.begin();
  for(QHash<QString, ChannelData>::Iterator end = channelDataMap.end(); it != end; ++it)
  {
    const ChannelData& channelData = it.value();
    if(channelData.tradesWidget && qobject_cast<QDockWidget*>(channelData.tradesWidget->parent()) == sender)
      goto found;
  }
  return;
found:
  const QString& channelName = it.key();
  if(channelName == dataModel.getMarketName())
    return;
  ChannelData& channelData = it.value();
  bool active = (channelData.graphWidget && qobject_cast<QDockWidget*>(channelData.graphWidget->parent())->isVisible()) ||
                (channelData.tradesWidget && qobject_cast<QDockWidget*>(channelData.tradesWidget->parent())->isVisible());
  if(active)
    dataService.subscribe(channelName);
  else
    dataService.unsubscribe(channelName);
}

void MainWindow::enableGraphUpdates(bool enable)
{
  QDockWidget* sender = qobject_cast<QDockWidget*>(this->sender());
  QHash<QString, ChannelData>::Iterator it = channelDataMap.begin();
  for(QHash<QString, ChannelData>::Iterator end = channelDataMap.end(); it != end; ++it)
  {
    const ChannelData& channelData = it.value();
    if(channelData.graphWidget && qobject_cast<QDockWidget*>(channelData.graphWidget->parent()) == sender)
      goto found;
  }
  return;
found:
  const QString& channelName = it.key();
  if(channelName == dataModel.getMarketName())
    return;
  ChannelData& channelData = it.value();
  bool active = (channelData.graphWidget && qobject_cast<QDockWidget*>(channelData.graphWidget->parent())->isVisible()) ||
                (channelData.tradesWidget && qobject_cast<QDockWidget*>(channelData.tradesWidget->parent())->isVisible());
  if(active)
    dataService.subscribe(channelName);
  else
    dataService.unsubscribe(channelName);
}
