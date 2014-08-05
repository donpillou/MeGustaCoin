
#include "stdafx.h"

MarketsWidget::MarketsWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager, BotService& botService) :
  QWidget(&tabFramework), tabFramework(tabFramework), entityManager(entityManager), botService(botService), botMarketModel(entityManager), selectedMarketId(0)
{
  entityManager.registerListener<EBotService>(*this);

  setWindowTitle(tr("Markets"));

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
  connect(marketView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(marketSelectionChanged()));
  connect(&botMarketModel, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(marketDataChanged(const QModelIndex&, const QModelIndex&)));
  connect(&botMarketModel, SIGNAL(rowsRemoved(const QModelIndex&, int, int)), this, SLOT(marketDataRemoved(const QModelIndex&, int, int)));
  connect(&botMarketModel, SIGNAL(modelReset()), this, SLOT(marketDataReset()));
  connect(&botMarketModel, SIGNAL(rowsInserted(const QModelIndex&, int, int)), this, SLOT(marketDataAdded(const QModelIndex&, int, int)));

  QVBoxLayout* layout = new QVBoxLayout;
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->addWidget(toolBar);
  layout->addWidget(marketView);
  setLayout(layout);

  QHeaderView* headerView = marketView->header();
  headerView->resizeSection(0, 100);
  headerView->resizeSection(1, 60);
  marketView->sortByColumn(0);
  settings.beginGroup("Markets");
  headerView->restoreState(settings.value("HeaderState").toByteArray());
  selectedMarketId = settings.value("SelectedMarketId").toUInt();
  settings.endGroup();
  headerView->setStretchLastSection(false);
  headerView->setResizeMode(0, QHeaderView::Stretch);
}

MarketsWidget::~MarketsWidget()
{
  entityManager.unregisterListener<EBotService>(*this);
}

void MarketsWidget::saveState(QSettings& settings)
{
  settings.beginGroup("Markets");
  settings.setValue("HeaderState", marketView->header()->saveState());
  settings.setValue("SelectedMarketId", selectedMarketId);
  settings.endGroup();
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
  if(QMessageBox::question(this, tr("Delete Market"), tr("Do you really want to delete the selected market?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
    return;

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

  setWindowTitle(title);
  tabFramework.toggleViewAction(this)->setText(tr("Markets"));
}

void MarketsWidget::updateToolBarButtons()
{
  EBotService* eBotService = entityManager.getEntity<EBotService>(0);
  bool connected = eBotService->getState() == EBotService::State::connected;
  bool marketSelected = !selection.isEmpty();

  addAction->setEnabled(connected);
  //editAction->setEnabled(connected && marketSelected);
  removeAction->setEnabled(connected && marketSelected);
}

void MarketsWidget::updateSelection()
{
  QModelIndexList modelSelection = marketView->selectionModel()->selectedRows();
  selection.clear();
  if(!modelSelection.isEmpty())
  {
    QModelIndex modelIndex = proxyModel->mapToSource(modelSelection.front());
    EBotMarket* eBotMarket = (EBotMarket*)modelIndex.internalPointer();
    selection.insert(eBotMarket);
  }
  if(!selection.isEmpty())
    selectedMarketId = (*selection.begin())->getId();
  updateToolBarButtons();
}

void MarketsWidget::marketSelectionChanged()
{
  updateSelection();
  if(!selection.isEmpty())
    botService.selectMarket((*selection.begin())->getId());
}

void MarketsWidget::marketDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
  QModelIndex index = topLeft;
  for(int i = topLeft.row(), end = bottomRight.row();;)
  {
    EBotMarket* eBotMarket = (EBotMarket*)index.internalPointer();
    if(selection.contains(eBotMarket))
    {
      updateSelection();
      break;
    }
    if(i++ == end)
      break;
    index = index.sibling(i, 0);
  }
}

void MarketsWidget::marketDataRemoved(const QModelIndex& parent, int start, int end)
{
  for(int i = start;;)
  {
    QModelIndex index = botMarketModel.index(i, 0, parent);
    EBotMarket* eBotMarket = (EBotMarket*)index.internalPointer();
    selection.remove(eBotMarket);
    if(i++ == end)
      break;
  }
}

void MarketsWidget::marketDataReset()
{
  selection.clear();
}

void MarketsWidget::marketDataAdded(const QModelIndex& parent, int start, int end)
{
  if(selection.isEmpty() && selectedMarketId)
  {
    for(int i = start;;)
    {
      QModelIndex index = botMarketModel.index(i, 0, parent);
      EBotMarket* eBotMarket = (EBotMarket*)index.internalPointer();
      if(eBotMarket->getId() == selectedMarketId)
      {
        QModelIndex proxyIndex = proxyModel->mapFromSource(index);
        QModelIndex proxyIndexEnd = proxyModel->mapFromSource(botMarketModel.index(i, botMarketModel.columnCount(parent) - 1, parent));
        marketView->selectionModel()->select(QItemSelection(proxyIndex, proxyIndexEnd), QItemSelectionModel::Select);
        break;
      }
      if(i++ == end)
        break;
    }
  }
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
