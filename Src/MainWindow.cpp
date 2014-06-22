
#include "stdafx.h"

MainWindow::MainWindow() : settings(QSettings::IniFormat, QSettings::UserScope, "Meguco", "MegucoClient"),
  dataService(globalEntityManager), botService(globalEntityManager)
{
  globalEntityManager.delegateEntity(*new EBotService);
  globalEntityManager.delegateEntity(*new EDataService);
  globalEntityManager.registerListener<EBotMarketBalance>(*this);
  globalEntityManager.registerListener<EBotService>(*this);

  connect(&liveTradesSignalMapper, SIGNAL(mapped(const QString&)), this, SLOT(createLiveTradeWidget(const QString&)));
  connect(&liveGraphSignalMapper, SIGNAL(mapped(const QString&)), this, SLOT(createLiveGraphWidget(const QString&)));

  marketsWidget = new MarketsWidget(*this, settings, globalEntityManager, botService);
  ordersWidget = new OrdersWidget(*this, settings, globalEntityManager, botService, dataService);
  transactionsWidget = new TransactionsWidget(*this, settings, globalEntityManager, botService);
  //graphWidget = new GraphWidget(this, settings, 0, QString(), dataModel.getDataChannels());
  botsWidget = new BotsWidget(*this, settings, globalEntityManager, botService);
  botTransactionsWidget = new BotTransactionsWidget(*this, settings, globalEntityManager);
  botOrderWidget = new BotOrderWidget(*this, settings, globalEntityManager);
  botLogWidget = new BotLogWidget(*this, settings, globalEntityManager);
  logWidget = new LogWidget(this, settings, globalEntityManager);

  setWindowIcon(QIcon(":/Icons/bitcoin_big.png"));
  updateWindowTitle();
  resize(625, 400);

  addTab(marketsWidget);
  addTab(logWidget, QTabFramework::InsertBottom, marketsWidget);
  addTab(transactionsWidget, QTabFramework::InsertRight, marketsWidget);
  addTab(ordersWidget, QTabFramework::InsertOnTop, transactionsWidget);
  //addTab(graphWidget);
  addTab(botsWidget, QTabFramework::InsertOnTop, transactionsWidget);
  addTab(botTransactionsWidget, QTabFramework::InsertOnTop, transactionsWidget);
  addTab(botOrderWidget, QTabFramework::InsertOnTop, transactionsWidget);
  addTab(botLogWidget, QTabFramework::InsertOnTop, transactionsWidget);

  QMenuBar* menuBar = this->menuBar();
  QMenu* menu = menuBar->addMenu(tr("&Client"));
  connect(menu->addAction(tr("&Options...")), SIGNAL(triggered()), this, SLOT(showOptions()));
  menu->addSeparator();
  QAction* action = menu->addAction(tr("&Exit"));
  action->setShortcut(QKeySequence::Quit);
  connect(action, SIGNAL(triggered()), this, SLOT(close()));

  viewMenu = menuBar->addMenu(tr("&View"));
  connect(viewMenu, SIGNAL(aboutToShow()), this, SLOT(updateViewMenu()));

  //QMenu* toolsMenu = menuBar->addMenu(tr("&Tools"));
  //connect(toolsMenu->addAction(tr("&Options...")), SIGNAL(triggered()), this, SLOT(showOptions()));

  menu = menuBar->addMenu(tr("&Help"));
  connect(menu->addAction(tr("&About...")), SIGNAL(triggered()), this, SLOT(about()));
  connect(menu->addAction(tr("About &Qt...")), SIGNAL(triggered()), qApp, SLOT(aboutQt()));

  QStringList liveTradesWidgets = settings.value("LiveTradesWidgets").toStringList();
  QStringList liveGraphWidgets = settings.value("LiveGraphWidgets").toStringList();
  foreach(const QString& channel, liveTradesWidgets)
  {
    QStringList channelData = channel.split('\n');
    if(channelData.size() < 3)
      continue;
    createChannelData(channelData[0], channelData[1], channelData[2]);
    createLiveTradeWidget(channelData[0]);
  }
  foreach(const QString& channel, liveGraphWidgets)
  {
    QStringList channelData = channel.split('\n');
    if(channelData.size() < 3)
      continue;
    createChannelData(channelData[0], channelData[1], channelData[2]);
    createLiveGraphWidget(channelData[0]);
  }

  restoreLayout(settings.value("Layout").toByteArray());
  //restoreGeometry(settings.value("Geometry").toByteArray());
  //restoreState(settings.value("WindowState").toByteArray());

  startDataService();
  startBotService();
}

MainWindow::~MainWindow()
{
  globalEntityManager.unregisterListener<EBotMarketBalance>(*this);
  globalEntityManager.unregisterListener<EBotService>(*this);

  dataService.stop();
  botService.stop();

  //// manually delete widgets since they hold a reference to the data model
  //for(QHash<QString, ChannelData>::Iterator i = channelDataMap.begin(), end = channelDataMap.end(); i != end; ++i)
  //{
  //  ChannelData& channelData = i.value();
  //  delete channelData.tradesWidget;
  //  delete channelData.graphWidget;
  //}
  //delete marketsWidget;
  //delete ordersWidget;
  //delete transactionsWidget;
  ////delete graphWidget;
  //delete botsWidget;
  //delete logWidget;

  qDeleteAll(channelGraphModels);
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

void MainWindow::closeEvent(QCloseEvent* event)
{
  settings.setValue("Layout", saveLayout());
  ordersWidget->saveState(settings);
  transactionsWidget->saveState(settings);
  //graphWidget->saveState(settings);
  botsWidget->saveState(settings);

  QStringList openedLiveTradesWidgets;
  QStringList openedLiveGraphWidgets;
  for(QHash<QString, ChannelData>::iterator i = channelDataMap.begin(), end = channelDataMap.end(); i != end; ++i)
  {
    const QString& channelName = i.key();
    ChannelData& channelData = i.value();
    EDataSubscription* eDataSubscription = channelData.channelEntityManager.getEntity<EDataSubscription>(0);
    if(!eDataSubscription)
      continue;
    if(channelData.tradesWidget)
    {
      channelData.tradesWidget->saveState(settings);
      if(isVisible(channelData.tradesWidget))
        openedLiveTradesWidgets.append(channelName + "\n" + eDataSubscription->getBaseCurrency() + "\n" + eDataSubscription->getCommCurrency());
    }
    if(channelData.graphWidget)
    {
      channelData.graphWidget->saveState(settings);
      if(isVisible(channelData.graphWidget))
        openedLiveGraphWidgets.append(channelName + "\n" + eDataSubscription->getBaseCurrency() + "\n" + eDataSubscription->getCommCurrency());
    }
  }

  logWidget->saveState(settings);
  settings.setValue("LiveTradesWidgets", openedLiveTradesWidgets);
  settings.setValue("LiveGraphWidgets", openedLiveGraphWidgets);

  QTabFramework::closeEvent(event);
}

void MainWindow::updateWindowTitle()
{
  EBotMarketBalance* eBotMarketBalance = globalEntityManager.getEntity<EBotMarketBalance>(0);
  EBotMarketAdapter* eBotMarketAdapater = 0;
  if(eBotMarketBalance)
  {
    EBotService* eBotService = globalEntityManager.getEntity<EBotService>(0);
    EBotMarket* eBotMarket = globalEntityManager.getEntity<EBotMarket>(eBotService->getSelectedMarketId());
    if(eBotMarket)
      eBotMarketAdapater = globalEntityManager.getEntity<EBotMarketAdapter>(eBotMarket->getMarketAdapterId());
  }
  if(!eBotMarketBalance || !eBotMarketAdapater)
    setWindowTitle(tr("Meguco Client"));
  else
  {
    EBotService* eBotService = globalEntityManager.getEntity<EBotService>(0);
    EBotMarket* eBotMarket = globalEntityManager.getEntity<EBotMarket>(eBotService->getSelectedMarketId());
    EBotMarketAdapter* eBotMarketAdapater = 0;
    if(eBotMarket)
      eBotMarketAdapater = globalEntityManager.getEntity<EBotMarketAdapter>(eBotMarket->getMarketAdapterId());

    double usd = eBotMarketBalance->getAvailableUsd() + eBotMarketBalance->getReservedUsd();
    double btc = eBotMarketBalance->getAvailableBtc() + eBotMarketBalance->getReservedBtc();
    QString title = QString("%1(%2) %3 / %4(%5) %6 - ").arg(
      eBotMarketAdapater->formatPrice(eBotMarketBalance->getAvailableUsd()), 
      eBotMarketAdapater->formatPrice(usd), 
      eBotMarketAdapater->getBaseCurrency(),
      eBotMarketAdapater->formatAmount(eBotMarketBalance->getAvailableBtc()), 
      eBotMarketAdapater->formatAmount(btc), 
      eBotMarketAdapater->getCommCurrency());
    if(eBotMarketAdapater)
    {
      const QString& marketName = eBotMarketAdapater->getName();
      title += marketName;
      Entity::Manager* channelEntityManager = dataService.getSubscription(marketName);
      if(channelEntityManager)
      {
        EDataSubscription* eDataSubscription = channelEntityManager->getEntity<EDataSubscription>(0);
        EDataTickerData* eDataTickerData = channelEntityManager->getEntity<EDataTickerData>(0);
        if(eDataSubscription && eDataTickerData)
          title += QString(" - %1 bid / %2 ask").arg(eDataSubscription->formatPrice(eDataTickerData->getBid()), eDataSubscription->formatPrice(eDataTickerData->getAsk()));
      }
    }
    setWindowTitle(title);
  }
}

void MainWindow::updateViewMenu()
{
  viewMenu->clear();

  //QAction* action = viewMenu->addAction(tr("&Refresh"));
  //action->setShortcut(QKeySequence(QKeySequence::Refresh));
  //connect(action, SIGNAL(triggered()), this, SLOT(refresh()));
  //viewMenu->addSeparator();
  viewMenu->addAction(toggleViewAction(marketsWidget));
  viewMenu->addAction(toggleViewAction(ordersWidget));
  viewMenu->addAction(toggleViewAction(transactionsWidget));
  //viewMenu->addAction(toggleViewAction(graphWidget));
  viewMenu->addAction(toggleViewAction(logWidget));
  viewMenu->addAction(toggleViewAction(botsWidget));
  viewMenu->addAction(toggleViewAction(botTransactionsWidget));
  viewMenu->addAction(toggleViewAction(botOrderWidget));
  viewMenu->addAction(toggleViewAction(botLogWidget));
  viewMenu->addSeparator();
  QList<EDataMarket*> channels;
  globalEntityManager.getAllEntities<EDataMarket>(channels);
  for(QList<EDataMarket*>::Iterator i = channels.begin(), end = channels.end(); i != end; ++i)
  {
    const QString& channelName = (*i)->getName();
    QMenu* subMenu = viewMenu->addMenu(channelName);

    QHash<QString, ChannelData>::Iterator it = channelDataMap.find(channelName);
    ChannelData* channelData = it == channelDataMap.end() ? 0 : &it.value();

    if(channelData && channelData->tradesWidget)
      subMenu->addAction(toggleViewAction(channelData->tradesWidget));
    else
    {
      QAction* action = subMenu->addAction(tr("Live Trades"));
      liveTradesSignalMapper.setMapping(action, channelName);
      connect(action, SIGNAL(triggered()), &liveTradesSignalMapper, SLOT(map()));
    }
    if(channelData && channelData->graphWidget)
      subMenu->addAction(toggleViewAction(channelData->graphWidget));
    else
    {
      QAction* action = subMenu->addAction(tr("Live Graph"));
      liveGraphSignalMapper.setMapping(action, channelName);
      connect(action, SIGNAL(triggered()), &liveGraphSignalMapper, SLOT(map()));
    }
  }
}

void MainWindow::createChannelData(const QString& channelName, const QString& currencyBase, const QString currencyComm)
{
  QHash<QString, ChannelData>::Iterator it = channelDataMap.find(channelName);
  if(it != channelDataMap.end())
    return;

  ChannelData& channelData = *channelDataMap.insert(channelName, ChannelData());
  channelData.channelName = channelName;
  channelData.channelEntityManager.delegateEntity(*new EDataSubscription(currencyBase, currencyComm));

  if(!channelGraphModels.contains(channelName))
    channelGraphModels.insert(channelName, new GraphModel(channelData.channelEntityManager));
}

MainWindow::ChannelData* MainWindow::getChannelData(const QString& channelName)
{
  QHash<QString, ChannelData>::Iterator it = channelDataMap.find(channelName);
  if(it == channelDataMap.end())
  {
    QList<EDataMarket*> channels;
    EDataMarket* eDataMarket = 0;
    globalEntityManager.getAllEntities<EDataMarket>(channels);
    for(QList<EDataMarket*>::Iterator i = channels.begin(), end = channels.end(); i != end; ++i)
      if((*i)->getName() == channelName)
        eDataMarket = *i;
    if(!eDataMarket)
      return 0;

    ChannelData& channelData = *channelDataMap.insert(channelName, ChannelData());
    channelData.channelName = channelName;
    channelData.channelEntityManager.delegateEntity(*new EDataSubscription(eDataMarket->getBaseCurrency(), eDataMarket->getCommCurrency()));

    if(!channelGraphModels.contains(channelName))
      channelGraphModels.insert(channelName, new GraphModel(channelData.channelEntityManager));

    return &channelData;
  }
  else
    return &*it;
}

void MainWindow::updateChannelSubscription(ChannelData& channelData)
{
  bool active = channelData.channelName == selectedChannelName ||
    (channelData.graphWidget && isVisible(channelData.graphWidget)) ||
    (channelData.tradesWidget && isVisible(channelData.tradesWidget));
  if(active)
    dataService.subscribe(channelData.channelName, channelData.channelEntityManager);
  else
    dataService.unsubscribe(channelData.channelName);
}

void MainWindow::createLiveTradeWidget(const QString& channelName)
{
  ChannelData* channelData = getChannelData(channelName);
  if(!channelData)
    return;
  if(channelData->tradesWidget)
    return;
  channelData->tradesWidget = new TradesWidget(*this, settings, channelName, channelData->channelEntityManager);
  channelData->tradesWidget->setObjectName(channelName + "LiveTrades");
  addTab(channelData->tradesWidget);
  connect(toggleViewAction(channelData->tradesWidget), SIGNAL(toggled(bool)), this, SLOT(enableTradesUpdates(bool)));
  channelData->tradesWidget->updateTitle();
  
  dataService.subscribe(channelName, channelData->channelEntityManager);
}

void MainWindow::createLiveGraphWidget(const QString& channelName)
{
  ChannelData* channelData = getChannelData(channelName);
  if(!channelData)
    return;
  if(channelData->graphWidget)
    return;
  GraphModel* channelGraphModel = channelGraphModels[channelName];
  Q_ASSERT(channelGraphModel);

  channelData->graphWidget = new GraphWidget(*this, settings, channelName, channelName + "_0", globalEntityManager, channelData->channelEntityManager, *channelGraphModel, channelGraphModels);
  channelData->graphWidget->setObjectName(channelName + "LiveGraph");
  addTab(channelData->graphWidget);
  connect(toggleViewAction(channelData->graphWidget), SIGNAL(toggled(bool)), this, SLOT(enableGraphUpdates(bool)));
  channelData->graphWidget->updateTitle();

  dataService.subscribe(channelName, channelData->channelEntityManager);
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
  QAction* sender = qobject_cast<QAction*>(this->sender());
  QHash<QString, ChannelData>::Iterator it = channelDataMap.begin();
  for(QHash<QString, ChannelData>::Iterator end = channelDataMap.end(); it != end; ++it)
  {
    ChannelData& channelData = it.value();
    if(channelData.tradesWidget && toggleViewAction(channelData.tradesWidget) == sender)
    {
      updateChannelSubscription(channelData);
      return;
    }
  }
}

void MainWindow::enableGraphUpdates(bool enable)
{
  QAction* sender = qobject_cast<QAction*>(this->sender());
  QHash<QString, ChannelData>::Iterator it = channelDataMap.begin();
  for(QHash<QString, ChannelData>::Iterator end = channelDataMap.end(); it != end; ++it)
  {
    ChannelData& channelData = it.value();
    if(channelData.graphWidget && toggleViewAction(channelData.graphWidget) == sender)
    {
      updateChannelSubscription(channelData);
      return;
    }
  }
}

void MainWindow::addedEntity(Entity& entity)
{
  switch((EType)entity.getType())
  {
  case EType::botMarketBalance:
  case EType::dataTickerData:
    updateWindowTitle();
    break;
  default:
    break;
  }
}

void MainWindow::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
{
  switch((EType)newEntity.getType())
  {
  case EType::botMarketBalance:
  case EType::dataTickerData:
    updateWindowTitle();
    break;
  case EType::botService:
    {
      QString oldSelectedChannelName = selectedChannelName;
      selectedChannelName.clear();
      EBotService* eBotService = dynamic_cast<EBotService*>(&newEntity);
      if(eBotService && eBotService->getSelectedMarketId() != 0)
      {
        EBotMarket* eBotMarket = globalEntityManager.getEntity<EBotMarket>(eBotService->getSelectedMarketId());
        if(eBotMarket)
        {
          EBotMarketAdapter* eBotMarketAdpater = globalEntityManager.getEntity<EBotMarketAdapter>(eBotMarket->getMarketAdapterId());
          if(eBotMarketAdpater)
            selectedChannelName = eBotMarketAdpater->getName();
        }
      }
      if(selectedChannelName != oldSelectedChannelName)
      {
        if(!oldSelectedChannelName.isEmpty())
        {
          ChannelData* channelData = getChannelData(oldSelectedChannelName);
          if(channelData)
          {
            updateChannelSubscription(*channelData);
            channelData->channelEntityManager.unregisterListener<EDataTickerData>(*this);
          }
        }
        if(!selectedChannelName.isEmpty())
        {
            ChannelData* channelData = getChannelData(selectedChannelName);
            if(channelData)
            {
              dataService.subscribe(selectedChannelName, channelData->channelEntityManager);
              channelData->channelEntityManager.registerListener<EDataTickerData>(*this);
            }
        }
      }
    }
    break;
  default:
    break;
  }
}

void MainWindow::removedAll(quint32 type)
{
  switch((EType)type)
  {
  case EType::botMarketBalance:
  case EType::dataTickerData:
    updateWindowTitle();
    break;
  default:
    break;
  }
}
