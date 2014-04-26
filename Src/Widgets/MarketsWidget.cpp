
#include "stdafx.h"

MarketsWidget::MarketsWidget(QWidget* parent, QSettings& settings, Entity::Manager& entityManager, BotService& botService) :
  QWidget(parent), entityManager(entityManager), botService(botService), botMarketModel(entityManager)
{
  entityManager.registerListener<EBotService>(*this);

  QToolBar* toolBar = new QToolBar(this);
  toolBar->setStyleSheet("QToolBar { border: 0px }");
  toolBar->setIconSize(QSize(16, 16));
  toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  addAction = toolBar->addAction(QIcon(":/Icons/cart_add.png"), tr("&Add"));
  connect(addAction, SIGNAL(triggered()), this, SLOT(addMarket()));
  editAction = toolBar->addAction(QIcon(":/Icons/cart_edit.png"), tr("&Edit"));
  editAction->setEnabled(false);
  connect(editAction, SIGNAL(triggered()), this, SLOT(editMarket()));
  removeAction = toolBar->addAction(QIcon(":/Icons/cancel2.png"), tr("&Remove"));
  removeAction->setEnabled(false);
  connect(removeAction, SIGNAL(triggered()), this, SLOT(removeMarket()));

  marketView = new QTreeView(this);
  marketView->setUniformRowHeights(true);
  proxyModel = new QSortFilterProxyModel(this);
  proxyModel->setDynamicSortFilter(true);
  proxyModel->setSourceModel(&botMarketModel);
  marketView->setModel(proxyModel);
  marketView->setSortingEnabled(true);
  marketView->setRootIsDecorated(false);
  marketView->setAlternatingRowColors(true);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->addWidget(toolBar);
  layout->addWidget(marketView);
  setLayout(layout);

  connect(marketView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), this, SLOT(updateToolBarButtons()));

  //QHeaderView* headerView = orderView->header();
  //headerView->resizeSection(0, 50);
  //headerView->resizeSection(1, 60);
  //headerView->resizeSection(2, 110);
  //headerView->resizeSection(3, 85);
  //headerView->resizeSection(4, 100);
  //headerView->resizeSection(5, 85);
  //headerView->resizeSection(5, 85);
  //headerView->setStretchLastSection(false);
  //headerView->setResizeMode(0, QHeaderView::Stretch);
  //orderView->sortByColumn(2);
  //settings.beginGroup("Orders");
  //headerView->restoreState(settings.value("HeaderState").toByteArray());
  //settings.endGroup();
}

MarketsWidget::~MarketsWidget()
{
  entityManager.unregisterListener<EBotService>(*this);
}

void MarketsWidget::saveState(QSettings& settings)
{
  //settings.beginGroup("Orders");
  //settings.setValue("HeaderState", orderView->header()->saveState());
  //settings.endGroup();
}

void MarketsWidget::addMarket()
{
  MarketDialog marketDialog(this, entityManager);
  if(marketDialog.exec() != QDialog::Accepted)
    return;

  botService.createMarket(marketDialog.getMarketAdapterId(),  marketDialog.getUserName(), marketDialog.getKey(), marketDialog.getSecret());
}

void MarketsWidget::editMarket()
{
}

void MarketsWidget::removeMarket()
{
  QModelIndexList selection = marketView->selectionModel()->selectedRows();
  foreach(const QModelIndex& proxyIndex, selection)
  {
    QModelIndex index = proxyModel->mapToSource(proxyIndex);
    EBotMarket* eBotMarket = (EBotMarket*)index.internalPointer();
    botService.removeMarket(eBotMarket->getId());
  }
}

void MarketsWidget::updateTitle(EBotService& eBotService)
{
  QString stateStr = eBotService.getStateName();

  QString title;
  if(stateStr.isEmpty())
    title = tr("Markets");
  else
    title = tr("Markets (%1)").arg(stateStr);

  QDockWidget* dockWidget = qobject_cast<QDockWidget*>(parent());
  dockWidget->setWindowTitle(title);
  dockWidget->toggleViewAction()->setText(tr("Markets"));
}

void MarketsWidget::updateToolBarButtons()
{
  bool connected = botService.isConnected();
  QModelIndexList selection = marketView->selectionModel()->selectedRows();
  bool marketSelected = !selection.isEmpty();

  addAction->setEnabled(connected);
  //editAction->setEnabled(connected && marketSelected);
  removeAction->setEnabled(connected && marketSelected);
}


void MarketsWidget::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
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
