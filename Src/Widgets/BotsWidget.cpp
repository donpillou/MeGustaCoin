
#include "stdafx.h"

BotsWidget::BotsWidget(QWidget* parent, QSettings& settings, Entity::Manager& entityManager, BotService& botService) :
  QWidget(parent), entityManager(entityManager),  botService(botService), orderModel(entityManager), transactionModel(entityManager)
{
  entityManager.registerListener<EBotService>(*this);

  //botsModel.addBot("BuyBot", *new BuyBot);

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
  simulateAction->setCheckable(true);
  connect(simulateAction, SIGNAL(triggered(bool)), this, SLOT(simulate(bool)));

  activateAction = toolBar->addAction(QIcon(":/Icons/user_gray_go.png"), tr("&Activate"));
  activateAction->setEnabled(false);
  activateAction->setCheckable(true);
  connect(activateAction, SIGNAL(triggered(bool)), this, SLOT(activate(bool)));

  orderView = new QTreeView(this);
  orderView->setUniformRowHeights(true);
  OrderSortProxyModel2* proxyModel = new OrderSortProxyModel2(this, orderModel);
  proxyModel->setDynamicSortFilter(true);
  proxyModel->setSourceModel(&orderModel);
  orderView->setModel(proxyModel);
  orderView->setSortingEnabled(true);
  orderView->setRootIsDecorated(false);
  orderView->setAlternatingRowColors(true);
  orderView->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::DoubleClicked);
  orderView->setSelectionMode(QAbstractItemView::ExtendedSelection);

  transactionView = new QTreeView(this);
  transactionView->setUniformRowHeights(true);
  TransactionSortProxyModel2* tproxyModel = new TransactionSortProxyModel2(this, transactionModel);
  tproxyModel->setDynamicSortFilter(true);
  tproxyModel->setSourceModel(&transactionModel);
  transactionView->setModel(tproxyModel);
  transactionView->setSortingEnabled(true);
  transactionView->setRootIsDecorated(false);
  transactionView->setAlternatingRowColors(true);

  splitter = new QSplitter(Qt::Vertical, this);
  splitter->setHandleWidth(1);
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
  QList<EBotEngine*> engines;
  entityManager.getAllEntities<EBotEngine>(engines);

  BotDialog botDialog(this, engines);
  if(botDialog.exec() != QDialog::Accepted)
    return;
  
  //botService.createSession(botDialog.getName(), botDialog.getEngine());
}

void BotsWidget::activate(bool enable)
{

}

void BotsWidget::simulate(bool enable)
{

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
    title = tr("Bots (%2)").arg(stateStr);

  QDockWidget* dockWidget = qobject_cast<QDockWidget*>(parent());
  dockWidget->setWindowTitle(title);
  dockWidget->toggleViewAction()->setText(tr("Bots"));
}

void BotsWidget::updateToolBarButtons()
{
  bool connected = botService.isConnected();
  addAction->setEnabled(connected);
  //optimizeAction->setEnabled(connected);
  //simulateAction->setEnabled(connected);
  //activateAction->setEnabled(connected);
}

void BotsWidget::updatedEntitiy(Entity& entity)
{
  switch ((EType)entity.getType())
  {
  case EType::botService:
    updateTitle(*dynamic_cast<EBotService*>(&entity));
    updateToolBarButtons();
    break;
  default:
    break;
  }
}
