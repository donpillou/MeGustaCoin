
#include "stdafx.h"

BotsWidget::BotsWidget(QWidget* parent, QSettings& settings, DataModel& dataModel, BotService& botService) :
  QWidget(parent), dataModel(dataModel),  botService(botService)
{
  connect(&dataModel.botsModel, SIGNAL(changedState()), this, SLOT(updateTitle()));
  connect(&dataModel.botsModel, SIGNAL(changedState()), this, SLOT(updateToolBarButtons()));

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
  OrderSortProxyModel* proxyModel = new OrderSortProxyModel(this, dataModel.botOrderModel);
  proxyModel->setDynamicSortFilter(true);
  proxyModel->setSourceModel(&dataModel.botOrderModel);
  orderView->setModel(proxyModel);
  orderView->setSortingEnabled(true);
  orderView->setRootIsDecorated(false);
  orderView->setAlternatingRowColors(true);
  orderView->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::DoubleClicked);
  orderView->setSelectionMode(QAbstractItemView::ExtendedSelection);

  transactionView = new QTreeView(this);
  transactionView->setUniformRowHeights(true);
  TransactionSortProxyModel* tproxyModel = new TransactionSortProxyModel(this, dataModel.botTransactionModel);
  tproxyModel->setDynamicSortFilter(true);
  tproxyModel->setSourceModel(&dataModel.botTransactionModel);
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

void BotsWidget::saveState(QSettings& settings)
{
  settings.beginGroup("Bots");
  //settings.setValue("HeaderState", botsView->header()->saveState());
  settings.endGroup();
}

void BotsWidget::addBot()
{
  BotDialog botDialog(this);
  if(botDialog.exec() != QDialog::Accepted)
    return;
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

void BotsWidget::updateTitle()
{
  QString stateStr = dataModel.botsModel.getStateName();

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
