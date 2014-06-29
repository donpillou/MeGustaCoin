
#include "stdafx.h"

BotsWidget::BotsWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager, BotService& botService) :
  QWidget(&tabFramework), tabFramework(tabFramework), entityManager(entityManager),  botService(botService), botSessionModel(entityManager), orderModel(entityManager), transactionModel(entityManager)
{
  entityManager.registerListener<EBotService>(*this);

  setWindowTitle(tr("Bot Sessions"));

  QToolBar* toolBar = new QToolBar(this);
  toolBar->setStyleSheet("QToolBar { border: 0px }");
  toolBar->setIconSize(QSize(16, 16));
  toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

  addAction = toolBar->addAction(QIcon(":/Icons/user_gray_add.png"), tr("&Add"));
  addAction->setEnabled(false);
  connect(addAction, SIGNAL(triggered()), this, SLOT(addBot()));

  optimizeAction = toolBar->addAction(QIcon(":/Icons/chart_curve.png"), tr("&Optimize"));
  optimizeAction->setEnabled(false);
  connect(optimizeAction, SIGNAL(triggered()), this, SLOT(optimize()));

  simulateAction = toolBar->addAction(QIcon(":/Icons/user_gray_go_gray.png"), tr("&Simulate"));
  simulateAction->setEnabled(false);
  connect(simulateAction, SIGNAL(triggered()), this, SLOT(simulate()));

  activateAction = toolBar->addAction(QIcon(":/Icons/user_gray_go.png"), tr("&Activate"));
  activateAction->setEnabled(false);
  connect(activateAction, SIGNAL(triggered()), this, SLOT(activate()));

  cancelAction = toolBar->addAction(QIcon(":/Icons/cancel2.png"), tr("&Cancel"));
  cancelAction->setEnabled(false);
  connect(cancelAction, SIGNAL(triggered()), this, SLOT(cancelBot()));

  sessionView = new QTreeView(this);
  sessionView->setUniformRowHeights(true);
  sessionProxyModel = new QSortFilterProxyModel(this);
  sessionProxyModel->setDynamicSortFilter(true);
  sessionProxyModel->setSourceModel(&botSessionModel);
  sessionView->setModel(sessionProxyModel);
  sessionView->setSortingEnabled(true);
  sessionView->setRootIsDecorated(false);
  sessionView->setAlternatingRowColors(true);
  connect(sessionView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(updateToolBarButtons()));
  connect(sessionView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(sessionSelectionChanged()));

  QVBoxLayout* layout = new QVBoxLayout;
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->addWidget(toolBar);
  layout->addWidget(sessionView);
  setLayout(layout);

  QHeaderView* headerView = sessionView->header();
  //headerView->resizeSection(0, 300);
  headerView->resizeSection((int)BotSessionModel::Column::balanceBase, 85);
  headerView->resizeSection((int)BotSessionModel::Column::balanceComm, 85);
  headerView->setStretchLastSection(false);
  headerView->setResizeMode(0, QHeaderView::Stretch);
  settings.beginGroup("BotSessions");
  headerView->restoreState(settings.value("HeaderState").toByteArray());
  settings.endGroup();
}

BotsWidget::~BotsWidget()
{
  entityManager.unregisterListener<EBotService>(*this);
}

void BotsWidget::saveState(QSettings& settings)
{
  settings.beginGroup("BotSessions");
  settings.setValue("HeaderState", sessionView->header()->saveState());
  settings.endGroup();
}

void BotsWidget::addBot()
{
  BotDialog botDialog(this, entityManager);
  if(botDialog.exec() != QDialog::Accepted)
    return;
  
  botService.createSession(botDialog.getName(), botDialog.getEngineId(), botDialog.getMarketId(), botDialog.getBalanceBase(), botDialog.getBalanceComm());
}

void BotsWidget::cancelBot()
{
  QModelIndexList selection = sessionView->selectionModel()->selectedRows();
  foreach(const QModelIndex& proxyIndex, selection)
  {
    QModelIndex index = sessionProxyModel->mapToSource(proxyIndex);
    EBotSession* eSession = (EBotSession*)index.internalPointer();
    if(eSession->getState() == EBotSession::State::stopped)
      botService.removeSession(eSession->getId());
    else
      botService.stopSession(eSession->getId());
  }
}

void BotsWidget::activate()
{
  QModelIndexList selection = sessionView->selectionModel()->selectedRows();
  foreach(const QModelIndex& proxyIndex, selection)
  {
    QModelIndex index = sessionProxyModel->mapToSource(proxyIndex);
    EBotSession* eSession = (EBotSession*)index.internalPointer();
    if(eSession->getState() == EBotSession::State::stopped)
      botService.startSession(eSession->getId());
  }
}

void BotsWidget::simulate()
{
  QModelIndexList selection = sessionView->selectionModel()->selectedRows();
  foreach(const QModelIndex& proxyIndex, selection)
  {
    QModelIndex index = sessionProxyModel->mapToSource(proxyIndex);
    EBotSession* eSession = (EBotSession*)index.internalPointer();
    if(eSession->getState() == EBotSession::State::stopped)
      botService.startSessionSimulation(eSession->getId());
  }
}

void BotsWidget::optimize()
{
  QModelIndexList selection = sessionView->selectionModel()->selectedRows();
  foreach(const QModelIndex& proxyIndex, selection)
  {
    QModelIndex index = sessionProxyModel->mapToSource(proxyIndex);
    EBotSession* eSession = (EBotSession*)index.internalPointer();
    if(eSession->getState() == EBotSession::State::stopped)
      botService.startSession(eSession->getId());
  }
}

void BotsWidget::updateTitle(EBotService& eBotService)
{
  QString stateStr = eBotService.getStateName();
  
  QString title;
  if(stateStr.isEmpty())
    title = tr("Bot Sessions");
  else
    title = tr("Bot Sessions (%1)").arg(stateStr);
  
  setWindowTitle(title);
  tabFramework.toggleViewAction(this)->setText(tr("Bot Sessions"));
}

void BotsWidget::updateToolBarButtons()
{
  EBotService* eBotService = entityManager.getEntity<EBotService>(0);
  bool connected = eBotService->getState() == EBotService::State::connected;
  addAction->setEnabled(connected);

  QModelIndexList selection = sessionView->selectionModel()->selectedRows();
  bool sessionSelected = !selection.isEmpty();
  bool sessionStopped = false;
  if(sessionSelected)
  {
    QModelIndex index = sessionProxyModel->mapToSource(selection.front());
    EBotSession* eSession = (EBotSession*)index.internalPointer();
    sessionStopped = eSession->getState() == EBotSession::State::stopped;
  }

  //optimizeAction->setEnabled(connected && sessionSelected && sessionStopped));
  simulateAction->setEnabled(connected && sessionSelected && sessionStopped);
  activateAction->setEnabled(connected && sessionSelected && sessionStopped);
  cancelAction->setEnabled(connected && sessionSelected);
}

void BotsWidget::sessionSelectionChanged()
{
  QModelIndexList selection = sessionView->selectionModel()->selectedRows();
  if(!selection.isEmpty())
  {
    QModelIndex modelIndex = sessionProxyModel->mapToSource(selection.front());
    EBotSession* eSession = (EBotSession*)modelIndex.internalPointer();
    botService.selectSession(eSession->getId());
  }
}

void BotsWidget::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
{
  EBotService* eBotService = dynamic_cast<EBotService*>(&newEntity);
  if(eBotService)
  {
    updateTitle(*eBotService);
    updateToolBarButtons();
    return;
  }
  Q_ASSERT(false);
}
