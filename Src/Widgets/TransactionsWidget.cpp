
#include "stdafx.h"

TransactionsWidget::TransactionsWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager, DataService& dataService) :
  QWidget(&tabFramework), tabFramework(tabFramework), entityManager(entityManager), dataService(dataService), transactionModel(entityManager)
{
  entityManager.registerListener<EBotService>(*this);

  setWindowTitle(tr("Transactions"));

  QToolBar* toolBar = new QToolBar(this);
  toolBar->setStyleSheet("QToolBar { border: 0px }");
  toolBar->setIconSize(QSize(16, 16));
  toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  refreshAction = toolBar->addAction(QIcon(":/Icons/arrow_refresh.png"), tr("&Refresh"));
  refreshAction->setEnabled(false);
  refreshAction->setShortcut(QKeySequence(QKeySequence::Refresh));
  connect(refreshAction, SIGNAL(triggered()), this, SLOT(refresh()));

  transactionView = new QTreeView(this);
  transactionView->setUniformRowHeights(true);
  proxyModel = new MarketTransactionSortProxyModel(this);
  proxyModel->setSourceModel(&transactionModel);
  proxyModel->setDynamicSortFilter(true);
  transactionView->setModel(proxyModel);
  transactionView->setSortingEnabled(true);
  transactionView->setRootIsDecorated(false);
  transactionView->setAlternatingRowColors(true);
  //transactionView->setSelectionMode(QAbstractItemView::ExtendedSelection);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->addWidget(toolBar);
  layout->addWidget(transactionView);
  setLayout(layout);

  QHeaderView* headerView = transactionView->header();
  headerView->resizeSection(0, 50);
  headerView->resizeSection(1, 110);
  headerView->resizeSection(2, 85);
  headerView->resizeSection(3, 100);
  headerView->resizeSection(4, 85);
  headerView->resizeSection(5, 75);
  headerView->resizeSection(6, 85);
  transactionView->sortByColumn(1);
  settings.beginGroup("Transactions");
  headerView->restoreState(settings.value("HeaderState").toByteArray());
  settings.endGroup();
  headerView->setStretchLastSection(false);
  headerView->setResizeMode(0, QHeaderView::Stretch);
}

TransactionsWidget::~TransactionsWidget()
{
  entityManager.unregisterListener<EBotService>(*this);
}

void TransactionsWidget::saveState(QSettings& settings)
{
  settings.beginGroup("Transactions");
  settings.setValue("HeaderState", transactionView->header()->saveState());
  settings.endGroup();
}

void TransactionsWidget::updateToolBarButtons()
{
  EBotService* eBotService = entityManager.getEntity<EBotService>(0);
  bool connected = eBotService->getState() == EBotService::State::connected;
  bool marketSelected = eBotService->getSelectedMarketId() != 0;

  refreshAction->setEnabled(connected && marketSelected);
}

void TransactionsWidget::refresh()
{
  dataService.refreshBrokerTransactions();
}

void TransactionsWidget::updateTitle(EBotService& eBotService)
{
  QString stateStr = eBotService.getStateName();
  QString title;
  if(stateStr.isEmpty())
    stateStr = eBotService.getMarketTransitionsState();
  if(stateStr.isEmpty())
    title = tr("Transactions");
  else
    title = tr("Transactions (%2)").arg(stateStr);

  setWindowTitle(title);
  tabFramework.toggleViewAction(this)->setText(tr("Transactions"));
}

void TransactionsWidget::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
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
