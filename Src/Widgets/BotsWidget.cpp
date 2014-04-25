
#include "stdafx.h"

BotsWidget::BotsWidget(QWidget* parent, QSettings& settings, Entity::Manager& entityManager, BotService& botService) :
  QWidget(parent), entityManager(entityManager),  botService(botService), botSessionModel(entityManager), orderModel(entityManager), transactionModel(entityManager)
{
  entityManager.registerListener<EBotService>(*this);

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

  orderView = new QTreeView(this);
  orderView->setUniformRowHeights(true);
  OrderSortProxyModel2* orderProxyModel = new OrderSortProxyModel2(this);
  orderProxyModel->setDynamicSortFilter(true);
  orderProxyModel->setSourceModel(&orderModel);
  orderView->setModel(orderProxyModel);
  orderView->setSortingEnabled(true);
  orderView->setRootIsDecorated(false);
  orderView->setAlternatingRowColors(true);
  orderView->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::DoubleClicked);
  orderView->setSelectionMode(QAbstractItemView::ExtendedSelection);

  transactionView = new QTreeView(this);
  transactionView->setUniformRowHeights(true);
  TransactionSortProxyModel2* transactionProxyModel = new TransactionSortProxyModel2(this);
  transactionProxyModel->setDynamicSortFilter(true);
  transactionProxyModel->setSourceModel(&transactionModel);
  transactionView->setModel(transactionProxyModel);
  transactionView->setSortingEnabled(true);
  transactionView->setRootIsDecorated(false);
  transactionView->setAlternatingRowColors(true);

  splitter = new QSplitter(Qt::Vertical, this);
  splitter->setHandleWidth(1);
  splitter->addWidget(sessionView);
  splitter->addWidget(orderView);
  splitter->addWidget(transactionView);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->addWidget(toolBar);
  layout->addWidget(splitter);
  setLayout(layout);

  //connect(botsView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), this, SLOT(updateToolBarButtons()));

  //QHeaderView* headerView = botsView->header();
  //headerView->resizeSection(0, 300);
  //headerView->resizeSection(1, 110);
  //headerView->setStretchLastSection(false);
  //headerView->setResizeMode(0, QHeaderView::Stretch);
  //settings.beginGroup("Bots");
  //headerView->restoreState(settings.value("HeaderState").toByteArray());
  //settings.endGroup();
}

BotsWidget::~BotsWidget()
{
  entityManager.unregisterListener<EBotService>(*this);
}

void BotsWidget::saveState(QSettings& settings)
{
  settings.beginGroup("Bots");
  //settings.setValue("HeaderState", botsView->header()->saveState());
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

}

void BotsWidget::simulate()
{
  QModelIndexList selection = sessionView->selectionModel()->selectedRows();
  foreach(const QModelIndex& proxyIndex, selection)
  {
    QModelIndex index = sessionProxyModel->mapToSource(proxyIndex);
    EBotSession* eSession = (EBotSession*)index.internalPointer();
    botService.startSessionSimulation(eSession->getId());
  }
}

void BotsWidget::optimize()
{
}

void BotsWidget::updateTitle(EBotService& eBotService)
{
  QString stateStr = eBotService.getStateName();

  QString title;
  if(stateStr.isEmpty())
    title = tr("Bots");
  else
    title = tr("Bots (%1)").arg(stateStr);

  QDockWidget* dockWidget = qobject_cast<QDockWidget*>(parent());
  dockWidget->setWindowTitle(title);
  dockWidget->toggleViewAction()->setText(tr("Bots"));
}

void BotsWidget::updateToolBarButtons()
{
  bool connected = botService.isConnected();
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

  //optimizeAction->setEnabled(connected && sessionSelected);
  simulateAction->setEnabled(connected && sessionSelected && sessionStopped);
  //activateAction->setEnabled(connected && sessionSelected);
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
  switch ((EType)newEntity.getType())
  {
  case EType::botService:
    updateTitle(*dynamic_cast<EBotService*>(&newEntity));
    updateToolBarButtons();
    break;
  default:
    break;
  }
}
