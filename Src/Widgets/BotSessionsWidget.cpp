
#include "stdafx.h"

BotSessionsWidget::BotSessionsWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager, BotService& botService) :
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

  //optimizeAction = toolBar->addAction(QIcon(":/Icons/chart_curve.png"), tr("&Optimize"));
  //optimizeAction->setEnabled(false);
  //connect(optimizeAction, SIGNAL(triggered()), this, SLOT(optimize()));

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
  proxyModel = new QSortFilterProxyModel(this);
  proxyModel->setDynamicSortFilter(true);
  proxyModel->setSourceModel(&botSessionModel);
  sessionView->setModel(proxyModel);
  sessionView->setSortingEnabled(true);
  sessionView->setRootIsDecorated(false);
  sessionView->setAlternatingRowColors(true);
  connect(sessionView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(sessionSelectionChanged()));
  connect(&botSessionModel, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(sessionDataChanged(const QModelIndex&, const QModelIndex&)));

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
  settings.beginGroup("BotSessions");
  headerView->restoreState(settings.value("HeaderState").toByteArray());
  settings.endGroup();
  headerView->setStretchLastSection(false);
  headerView->setResizeMode(0, QHeaderView::Stretch);
}

BotSessionsWidget::~BotSessionsWidget()
{
  entityManager.unregisterListener<EBotService>(*this);
}

void BotSessionsWidget::saveState(QSettings& settings)
{
  settings.beginGroup("BotSessions");
  settings.setValue("HeaderState", sessionView->header()->saveState());
  settings.endGroup();
}

void BotSessionsWidget::addBot()
{
  BotDialog botDialog(this, entityManager);
  if(botDialog.exec() != QDialog::Accepted)
    return;
  
  botService.createSession(botDialog.getName(), botDialog.getEngineId(), botDialog.getMarketId(), botDialog.getBalanceBase(), botDialog.getBalanceComm());
}

void BotSessionsWidget::cancelBot()
{
  QModelIndexList selection = sessionView->selectionModel()->selectedRows();
  foreach(const QModelIndex& proxyIndex, selection)
  {
    QModelIndex index = proxyModel->mapToSource(proxyIndex);
    EBotSession* eSession = (EBotSession*)index.internalPointer();
    if(eSession->getState() == EBotSession::State::stopped)
    {
      if(QMessageBox::question(this, tr("Delete Session"), tr("Do you really want to delete the selected session?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
          return;
      botService.removeSession(eSession->getId());
    }
    else
      botService.stopSession(eSession->getId());
  }
}

void BotSessionsWidget::activate()
{
  QModelIndexList selection = sessionView->selectionModel()->selectedRows();
  foreach(const QModelIndex& proxyIndex, selection)
  {
    QModelIndex index = proxyModel->mapToSource(proxyIndex);
    EBotSession* eSession = (EBotSession*)index.internalPointer();
    if(eSession->getState() == EBotSession::State::stopped)
      botService.startSession(eSession->getId());
  }
}

void BotSessionsWidget::simulate()
{
  QModelIndexList selection = sessionView->selectionModel()->selectedRows();
  foreach(const QModelIndex& proxyIndex, selection)
  {
    QModelIndex index = proxyModel->mapToSource(proxyIndex);
    EBotSession* eSession = (EBotSession*)index.internalPointer();
    if(eSession->getState() == EBotSession::State::stopped)
      botService.startSessionSimulation(eSession->getId());
  }
}

void BotSessionsWidget::optimize()
{
  QModelIndexList selection = sessionView->selectionModel()->selectedRows();
  foreach(const QModelIndex& proxyIndex, selection)
  {
    QModelIndex index = proxyModel->mapToSource(proxyIndex);
    EBotSession* eSession = (EBotSession*)index.internalPointer();
    if(eSession->getState() == EBotSession::State::stopped)
      botService.startSession(eSession->getId());
  }
}

void BotSessionsWidget::updateTitle(EBotService& eBotService)
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

void BotSessionsWidget::updateToolBarButtons()
{
  EBotService* eBotService = entityManager.getEntity<EBotService>(0);
  bool connected = eBotService->getState() == EBotService::State::connected;
  addAction->setEnabled(connected);

  bool sessionSelected = !selection.isEmpty();
  bool sessionStopped = false;
  if(sessionSelected)
  {
    EBotSession* eSession = *selection.begin();
    sessionStopped = eSession->getState() == EBotSession::State::stopped;
  }

  //optimizeAction->setEnabled(connected && sessionSelected && sessionStopped));
  simulateAction->setEnabled(connected && sessionSelected && sessionStopped);
  activateAction->setEnabled(connected && sessionSelected && sessionStopped);
  cancelAction->setEnabled(connected && sessionSelected);
}

void BotSessionsWidget::updateSelection()
{
  QModelIndexList modelSelection = sessionView->selectionModel()->selectedRows();
  selection.clear();
  if(!modelSelection.isEmpty())
  {
    QModelIndex modelIndex = proxyModel->mapToSource(modelSelection.front());
    EBotSession* eSession = (EBotSession*)modelIndex.internalPointer();
    selection.insert(eSession);
  }
  updateToolBarButtons();
}

void BotSessionsWidget::sessionSelectionChanged()
{
  updateSelection();
  if(!selection.isEmpty())
    botService.selectSession((*selection.begin())->getId());
}

void BotSessionsWidget::sessionDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
  QModelIndex index = topLeft;
  for(int i = topLeft.row(), end = bottomRight.row();;)
  {
    EBotSession* eBotSession = (EBotSession*)index.internalPointer();
    if(selection.contains(eBotSession))
    {
      updateSelection();
      break;
    }
    if(i++ == end)
      break;
    index = index.sibling(i, 0);
  }
}

void BotSessionsWidget::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
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
