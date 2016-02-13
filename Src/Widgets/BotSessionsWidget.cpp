
#include "stdafx.h"

BotSessionsWidget::BotSessionsWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager, DataService& dataService) :
  QWidget(&tabFramework), tabFramework(tabFramework), entityManager(entityManager),  dataService(dataService), sessionsModel(entityManager), /*ordersModel(entityManager), transactionModel(entityManager), */selectedSessionId(0)
{
  entityManager.registerListener<EConnection>(*this);

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
  proxyModel->setSourceModel(&sessionsModel);
  sessionView->setModel(proxyModel);
  sessionView->setSortingEnabled(true);
  sessionView->setRootIsDecorated(false);
  sessionView->setAlternatingRowColors(true);
  connect(sessionView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(sessionSelectionChanged()));
  connect(&sessionsModel, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(sessionDataChanged(const QModelIndex&, const QModelIndex&)));
  connect(&sessionsModel, SIGNAL(rowsRemoved(const QModelIndex&, int, int)), this, SLOT(sessionDataRemoved(const QModelIndex&, int, int)));
  connect(&sessionsModel, SIGNAL(modelReset()), this, SLOT(sessionDataReset()));
  connect(&sessionsModel, SIGNAL(rowsInserted(const QModelIndex&, int, int)), this, SLOT(sessionDataAdded(const QModelIndex&, int, int)));

  QVBoxLayout* layout = new QVBoxLayout;
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->addWidget(toolBar);
  layout->addWidget(sessionView);
  setLayout(layout);

  QHeaderView* headerView = sessionView->header();
  //headerView->resizeSection(0, 300);
  settings.beginGroup("BotSessions");
  headerView->restoreState(settings.value("HeaderState").toByteArray());
  selectedSessionId = settings.value("SelectedSessionId").toUInt();
  settings.endGroup();
  headerView->setStretchLastSection(false);
  headerView->setResizeMode(0, QHeaderView::Stretch);
}

BotSessionsWidget::~BotSessionsWidget()
{
  entityManager.unregisterListener<EConnection>(*this);
}

void BotSessionsWidget::saveState(QSettings& settings)
{
  settings.beginGroup("BotSessions");
  settings.setValue("HeaderState", sessionView->header()->saveState());
  settings.setValue("SelectedSessionId", selectedSessionId);
  settings.endGroup();
}

void BotSessionsWidget::addBot()
{
  BotDialog botDialog(this, entityManager);
  if(botDialog.exec() != QDialog::Accepted)
    return;
  
  dataService.createSession(botDialog.getName(), botDialog.getEngineId(), botDialog.getMarketId());
}

void BotSessionsWidget::cancelBot()
{
  QModelIndexList selection = sessionView->selectionModel()->selectedRows();
  foreach(const QModelIndex& proxyIndex, selection)
  {
    QModelIndex index = proxyModel->mapToSource(proxyIndex);
    EUserSession* eSession = (EUserSession*)index.internalPointer();
    if(eSession->getState() == EUserSession::State::stopped)
    {
      if(QMessageBox::question(this, tr("Delete Session"), tr("Do you really want to delete the selected session?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
          return;
      dataService.removeSession(eSession->getId());
    }
    else
      dataService.stopSession(eSession->getId());
  }
}

void BotSessionsWidget::activate()
{
  QModelIndexList selection = sessionView->selectionModel()->selectedRows();
  foreach(const QModelIndex& proxyIndex, selection)
  {
    QModelIndex index = proxyModel->mapToSource(proxyIndex);
    EUserSession* eSession = (EUserSession*)index.internalPointer();
    if(eSession->getState() == EUserSession::State::stopped)
      dataService.startSession(eSession->getId(), meguco_user_session_live);
  }
}

void BotSessionsWidget::simulate()
{
  QModelIndexList selection = sessionView->selectionModel()->selectedRows();
  foreach(const QModelIndex& proxyIndex, selection)
  {
    QModelIndex index = proxyModel->mapToSource(proxyIndex);
    EUserSession* eSession = (EUserSession*)index.internalPointer();
    if(eSession->getState() == EUserSession::State::stopped)
      dataService.startSession(eSession->getId(), meguco_user_session_simulation);
  }
}

void BotSessionsWidget::optimize()
{
  //QModelIndexList selection = sessionView->selectionModel()->selectedRows();
  //foreach(const QModelIndex& proxyIndex, selection)
  //{
  //  QModelIndex index = proxyModel->mapToSource(proxyIndex);
  //  EBotSession* eSession = (EBotSession*)index.internalPointer();
  //  if(eSession->getState() == EBotSession::State::stopped)
  //    dataService.startSession(eSession->getId());
  //}
}

void BotSessionsWidget::updateTitle(EConnection& eDataService)
{
  QString stateStr = eDataService.getStateName();
  
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
  EConnection* eDataService = entityManager.getEntity<EConnection>(0);
  bool connected = eDataService->getState() == EConnection::State::connected;
  addAction->setEnabled(connected);

  bool sessionSelected = !selection.isEmpty();
  bool sessionStopped = false;
  if(sessionSelected)
  {
    EUserSession* eSession = *selection.begin();
    sessionStopped = eSession->getState() == EUserSession::State::stopped;
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
    EUserSession* eSession = (EUserSession*)modelIndex.internalPointer();
    selection.insert(eSession);
  }
  if(!selection.isEmpty())
    selectedSessionId = (*selection.begin())->getId();
  updateToolBarButtons();
}

void BotSessionsWidget::sessionSelectionChanged()
{
  updateSelection();
  if(!selection.isEmpty())
    dataService.selectSession((*selection.begin())->getId());
}

void BotSessionsWidget::sessionDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
  QModelIndex index = topLeft;
  for(int i = topLeft.row(), end = bottomRight.row();;)
  {
    EUserSession* eBotSession = (EUserSession*)index.internalPointer();
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

void BotSessionsWidget::sessionDataRemoved(const QModelIndex& parent, int start, int end)
{
  for(int i = start;;)
  {
    QModelIndex index = sessionsModel.index(i, 0, parent);
    EUserSession* eBotSession = (EUserSession*)index.internalPointer();
    selection.remove(eBotSession);
    if(i++ == end)
      break;
  }
}

void BotSessionsWidget::sessionDataReset()
{
  selection.clear();
}

void BotSessionsWidget::sessionDataAdded(const QModelIndex& parent, int start, int end)
{
  if(selection.isEmpty() && selectedSessionId)
  {
    for(int i = start;;)
    {
      QModelIndex index = sessionsModel.index(i, 0, parent);
      EUserSession* eSession = (EUserSession*)index.internalPointer();
      if(eSession->getId() == selectedSessionId)
      {
        QModelIndex proxyIndex = proxyModel->mapFromSource(index);
        QModelIndex proxyIndexEnd = proxyModel->mapFromSource(sessionsModel.index(i, sessionsModel.columnCount(parent) - 1, parent));
        sessionView->selectionModel()->select(QItemSelection(proxyIndex, proxyIndexEnd), QItemSelectionModel::Select);
        break;
      }
      if(i++ == end)
        break;
    }
  }
}

void BotSessionsWidget::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
{
  EConnection* eDataService = dynamic_cast<EConnection*>(&newEntity);
  if(eDataService)
  {
    updateTitle(*eDataService);
    updateToolBarButtons();
    return;
  }
  Q_ASSERT(false);
}
