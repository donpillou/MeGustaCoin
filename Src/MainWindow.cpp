
#include "stdafx.h"

MainWindow::MainWindow() : settings(QSettings::IniFormat, QSettings::UserScope, "Meguco", "MegucoClientZ"),
  dataService(globalEntityManager)
{
  globalEntityManager.delegateEntity(*new EConnection);
  globalEntityManager.registerListener<EUserBrokerBalance>(*this);
  globalEntityManager.registerListener<EConnection>(*this);

  connect(&liveTradesSignalMapper, SIGNAL(mapped(const QString&)), this, SLOT(createLiveTradeWidget(const QString&)));
  connect(&liveGraphSignalMapper, SIGNAL(mapped(const QString&)), this, SLOT(createLiveGraphWidget(const QString&)));

  brokersWidget = new UserBrokersWidget(*this, settings, globalEntityManager, dataService);
  brokerOrdersWidget = new UserBrokerOrdersWidget(*this, settings, globalEntityManager, dataService);
  brokerTransactionsWidget = new UserBrokerTransactionsWidget(*this, settings, globalEntityManager, dataService);
  sessionsWidget = new UserSessionsWidget(*this, settings, globalEntityManager, dataService);
  sessionTransactionsWidget = new UserSessionTransactionsWidget(*this, settings, globalEntityManager);
  sessionAssetsWidget = new UserSessionAssetsWidget(*this, settings, globalEntityManager, dataService);
  sessionOrdersWidget = new UserSessionOrdersWidget(*this, settings, globalEntityManager);
  sessionPropertiesWidget = new UserSessionPropertiesWidget(*this, settings, globalEntityManager, dataService);
  sessionLogWidget = new UserSessionLogWidget(*this, settings, globalEntityManager);
  logWidget = new LogWidget(this, settings, globalEntityManager);
  processesWidget = new ProcessesWidget(this, settings, globalEntityManager);

  setWindowIcon(QIcon(":/Icons/bitcoin_big.png"));
  updateWindowTitle();
  resize(625, 400);

  addTab(brokersWidget);
  addTab(processesWidget, QTabFramework::InsertOnTop, brokersWidget); // todo: insert hidden
  addTab(logWidget, QTabFramework::InsertBottom, brokersWidget);
  addTab(brokerTransactionsWidget, QTabFramework::InsertRight, brokersWidget);
  addTab(brokerOrdersWidget, QTabFramework::InsertOnTop, brokerTransactionsWidget);
  addTab(sessionsWidget, QTabFramework::InsertOnTop, brokerTransactionsWidget);
  addTab(sessionTransactionsWidget, QTabFramework::InsertOnTop, brokerTransactionsWidget);
  addTab(sessionOrdersWidget, QTabFramework::InsertOnTop, brokerTransactionsWidget);
  addTab(sessionAssetsWidget, QTabFramework::InsertOnTop, brokerTransactionsWidget);
  addTab(sessionPropertiesWidget, QTabFramework::InsertOnTop, brokerTransactionsWidget);
  addTab(sessionLogWidget, QTabFramework::InsertOnTop, brokerTransactionsWidget);

  QMenuBar* menuBar = this->menuBar();
  QMenu* menu = menuBar->addMenu(tr("&Client"));
  connect(menu->addAction(tr("&Options...")), SIGNAL(triggered()), this, SLOT(showOptions()));
  menu->addSeparator();
  QAction* action = menu->addAction(tr("&Exit"));
  action->setShortcut(QKeySequence::Quit);
  connect(action, SIGNAL(triggered()), this, SLOT(close()));

  viewMenu = menuBar->addMenu(tr("&View"));
  connect(viewMenu, SIGNAL(aboutToShow()), this, SLOT(updateViewMenu()));

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

  startDataService();
  graphService.start();
}

MainWindow::~MainWindow()
{
  globalEntityManager.unregisterListener<EUserBrokerBalance>(*this);
  globalEntityManager.unregisterListener<EConnection>(*this);

  dataService.stop();
  graphService.stop();
}

void MainWindow::startDataService()
{
  settings.beginGroup("DataServer");
  QString dataServer = settings.value("Address", "127.0.0.1:40123").toString();
  QString userName = settings.value("User").toString();
  QString password = settings.value("Password").toString();
  dataService.start(dataServer, userName, password);
  settings.endGroup();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
  settings.setValue("Layout", saveLayout());
  brokerOrdersWidget->saveState(settings);
  brokerTransactionsWidget->saveState(settings);
  brokersWidget->saveState(settings);
  sessionsWidget->saveState(settings);
  sessionTransactionsWidget->saveState(settings);
  sessionAssetsWidget->saveState(settings);
  sessionOrdersWidget->saveState(settings);
  sessionPropertiesWidget->saveState(settings);
  sessionLogWidget->saveState(settings);
  logWidget->saveState(settings);
  processesWidget->saveState(settings);

  QStringList openedLiveTradesWidgets;
  QStringList openedLiveGraphWidgets;
  for(QHash<QString, ChannelData>::iterator i = channelDataMap.begin(), end = channelDataMap.end(); i != end; ++i)
  {
    const QString& channelName = i.key();
    ChannelData& channelData = i.value();
    EMarketSubscription* eSubscription = channelData.channelEntityManager->getEntity<EMarketSubscription>(0);
    if(!eSubscription)
      continue;
    if(channelData.tradesWidget)
    {
      channelData.tradesWidget->saveState(settings);
      if(isVisible(channelData.tradesWidget))
        openedLiveTradesWidgets.append(channelName + "\n" + eSubscription->getBaseCurrency() + "\n" + eSubscription->getCommCurrency());
    }
    if(channelData.graphWidget)
    {
      channelData.graphWidget->saveState(settings);
      if(isVisible(channelData.graphWidget))
        openedLiveGraphWidgets.append(channelName + "\n" + eSubscription->getBaseCurrency() + "\n" + eSubscription->getCommCurrency());
    }
  }

  settings.setValue("LiveTradesWidgets", openedLiveTradesWidgets);
  settings.setValue("LiveGraphWidgets", openedLiveGraphWidgets);

  QTabFramework::closeEvent(event);
}

void MainWindow::updateWindowTitle()
{
  EUserBrokerBalance* eUserBrokerBalance = globalEntityManager.getEntity<EUserBrokerBalance>(0);
  EBrokerType* eBrokerType = 0;
  if(eUserBrokerBalance)
  {
    EConnection* eDataService = globalEntityManager.getEntity<EConnection>(0);
    EUserBroker* eUserBroker = globalEntityManager.getEntity<EUserBroker>(eDataService->getSelectedBrokerId());
    if(eUserBroker)
      eBrokerType = globalEntityManager.getEntity<EBrokerType>(eUserBroker->getBrokerTypeId());
  }
  if(!eUserBrokerBalance || !eBrokerType)
    setWindowTitle(tr("Meguco Client"));
  else
  {
    EConnection* eDataService = globalEntityManager.getEntity<EConnection>(0);
    EUserBroker* eUserBroker = globalEntityManager.getEntity<EUserBroker>(eDataService->getSelectedBrokerId());
    EBrokerType* eBrokerType = 0;
    if(eUserBroker)
      eBrokerType = globalEntityManager.getEntity<EBrokerType>(eUserBroker->getBrokerTypeId());

    double usd = eUserBrokerBalance->getAvailableUsd() + eUserBrokerBalance->getReservedUsd();
    double btc = eUserBrokerBalance->getAvailableBtc() + eUserBrokerBalance->getReservedBtc();
    QString title = QString("%1(%2) %3 / %4(%5) %6 - ").arg(
      eBrokerType->formatPrice(eUserBrokerBalance->getAvailableUsd()), 
      eBrokerType->formatPrice(usd), 
      eBrokerType->getBaseCurrency(),
      eBrokerType->formatAmount(eUserBrokerBalance->getAvailableBtc()), 
      eBrokerType->formatAmount(btc), 
      eBrokerType->getCommCurrency());
    if(eBrokerType)
    {
      const QString& marketName = eBrokerType->getName();
      title += marketName;
      Entity::Manager* channelEntityManager = dataService.getSubscription(marketName);
      if(channelEntityManager)
      {
        EMarketSubscription* eDataSubscription = channelEntityManager->getEntity<EMarketSubscription>(0);
        EMarketTickerData* eDataTickerData = channelEntityManager->getEntity<EMarketTickerData>(0);
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

  viewMenu->addAction(toggleViewAction(brokersWidget));
  viewMenu->addAction(toggleViewAction(brokerOrdersWidget));
  viewMenu->addAction(toggleViewAction(brokerTransactionsWidget));
  viewMenu->addSeparator();
  viewMenu->addAction(toggleViewAction(sessionsWidget));
  viewMenu->addAction(toggleViewAction(sessionTransactionsWidget));
  viewMenu->addAction(toggleViewAction(sessionOrdersWidget));
  viewMenu->addAction(toggleViewAction(sessionAssetsWidget));
  viewMenu->addAction(toggleViewAction(sessionPropertiesWidget));
  viewMenu->addAction(toggleViewAction(sessionLogWidget));
  viewMenu->addSeparator();
  viewMenu->addAction(toggleViewAction(logWidget));
  viewMenu->addAction(toggleViewAction(processesWidget));
  viewMenu->addSeparator();
  QList<EMarket*> channels;
  globalEntityManager.getAllEntities<EMarket>(channels);
  for(QList<EMarket*>::Iterator i = channels.begin(), end = channels.end(); i != end; ++i)
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
  channelData.channelEntityManager = new Entity::Manager();
  channelData.graphModel = new GraphModel(channelName, globalEntityManager, *channelData.channelEntityManager, graphService);
  channelData.channelEntityManager->delegateEntity(*new EMarketSubscription(currencyBase, currencyComm));
}

MainWindow::ChannelData* MainWindow::getChannelData(const QString& channelName)
{
  QHash<QString, ChannelData>::Iterator it = channelDataMap.find(channelName);
  if(it == channelDataMap.end())
  {
    QList<EMarket*> channels;
    EMarket* eDataMarket = 0;
    globalEntityManager.getAllEntities<EMarket>(channels);
    for(QList<EMarket*>::Iterator i = channels.begin(), end = channels.end(); i != end; ++i)
      if((*i)->getName() == channelName)
        eDataMarket = *i;
    if(!eDataMarket)
      return 0;

    ChannelData& channelData = *channelDataMap.insert(channelName, ChannelData());
    channelData.channelEntityManager = new Entity::Manager();
    channelData.graphModel = new GraphModel(channelName, globalEntityManager, *channelData.channelEntityManager, graphService);
    channelData.channelEntityManager->delegateEntity(*new EMarketSubscription(eDataMarket->getBaseCurrency(), eDataMarket->getCommCurrency()));

    return &channelData;
  }
  else
    return &*it;
}

void MainWindow::updateChannelSubscription(ChannelData& channelData, bool enable)
{
  const QString& channelName = channelData.graphModel->getChannelName();
  bool active = enable || channelName == selectedChannelName ||
    (channelData.graphWidget && isVisible(channelData.graphWidget)) ||
    (channelData.tradesWidget && isVisible(channelData.tradesWidget));
  if(active)
    dataService.subscribe(channelName, *channelData.channelEntityManager);
  else
    dataService.unsubscribe(channelName);
}

void MainWindow::createLiveTradeWidget(const QString& channelName)
{
  ChannelData* channelData = getChannelData(channelName);
  if(!channelData)
    return;
  if(channelData->tradesWidget)
    return;
  channelData->tradesWidget = new TradesWidget(*this, settings, channelName, *channelData->channelEntityManager);
  channelData->tradesWidget->setObjectName(channelName + "LiveTrades");
  addTab(channelData->tradesWidget);
  connect(toggleViewAction(channelData->tradesWidget), SIGNAL(toggled(bool)), this, SLOT(enableTradesUpdates(bool)));
  channelData->tradesWidget->updateTitle();
  
  dataService.subscribe(channelName, *channelData->channelEntityManager);
}

void MainWindow::createLiveGraphWidget(const QString& channelName)
{
  ChannelData* channelData = getChannelData(channelName);
  if(!channelData)
    return;
  if(channelData->graphWidget)
    return;

  channelData->graphWidget = new GraphWidget(*this, settings, channelName, channelName + "_0", *channelData->channelEntityManager, *channelData->graphModel);
  channelData->graphWidget->setObjectName(channelName + "LiveGraph");
  addTab(channelData->graphWidget);
  connect(toggleViewAction(channelData->graphWidget), SIGNAL(toggled(bool)), this, SLOT(enableGraphUpdates(bool)));
  channelData->graphWidget->updateTitle();

  dataService.subscribe(channelName, *channelData->channelEntityManager);
}

void MainWindow::showOptions()
{
  OptionsDialog optionsDialog(this, &settings);
  if(optionsDialog.exec() != QDialog::Accepted)
    return;

  startDataService();
}

void MainWindow::about()
{
  QMessageBox::about(this, "About", "MegucoClient - A front end for the Meguco Trade Framework<br><a href=\"https://github.com/donpillou/MegucoClient\">https://github.com/donpillou/MegucoClient</a><br><br>Released under the GNU General Public License Version 3<br><br>MegucoClient uses the following third-party libraries and components:<br>&nbsp;&nbsp;- Qt (GUI)<br>&nbsp;&nbsp;- ZlimDB (Database)<br>&nbsp;&nbsp;- <a href=\"http://www.famfamfam.com/lab/icons/silk/\">silk icons</a> by Mark James<br><br>-- Donald Pillou, 2013-2016");
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
      updateChannelSubscription(channelData, enable);
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
      updateChannelSubscription(channelData, enable);
      return;
    }
  }
}

void MainWindow::addedEntity(Entity& entity)
{
  switch((EType)entity.getType())
  {
  case EType::userBrokerBalance:
  case EType::marketTickerData:
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
  case EType::userBrokerBalance:
  case EType::marketTickerData:
    updateWindowTitle();
    break;
  case EType::connection:
    {
      QString oldSelectedChannelName = selectedChannelName;
      selectedChannelName.clear();
      EConnection* eDataService = dynamic_cast<EConnection*>(&newEntity);
      if(eDataService && eDataService->getSelectedBrokerId() != 0)
      {
        EUserBroker* eUserBroker = globalEntityManager.getEntity<EUserBroker>(eDataService->getSelectedBrokerId());
        if(eUserBroker)
        {
          EBrokerType* eBrokerType = globalEntityManager.getEntity<EBrokerType>(eUserBroker->getBrokerTypeId());
          if(eBrokerType)
            selectedChannelName = eBrokerType->getName();
        }
      }
      if(selectedChannelName != oldSelectedChannelName)
      {
        if(!oldSelectedChannelName.isEmpty())
        {
          ChannelData* channelData = getChannelData(oldSelectedChannelName);
          if(channelData)
          {
            updateChannelSubscription(*channelData, false);
            channelData->channelEntityManager->unregisterListener<EMarketTickerData>(*this);
          }
        }
        if(!selectedChannelName.isEmpty())
        {
            ChannelData* channelData = getChannelData(selectedChannelName);
            if(channelData)
            {
              dataService.subscribe(selectedChannelName, *channelData->channelEntityManager);
              channelData->channelEntityManager->registerListener<EMarketTickerData>(*this);
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
  case EType::userBrokerBalance:
  case EType::marketTickerData:
    updateWindowTitle();
    break;
  default:
    break;
  }
}
