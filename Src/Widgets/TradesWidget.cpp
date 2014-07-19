
#include "stdafx.h"

TradesWidget::TradesWidget(QTabFramework& tabFramework, QSettings& settings, const QString& channelName, Entity::Manager& channelEntityManager) :
  QWidget(&tabFramework), tabFramework(tabFramework),
  channelName(channelName), channelEntityManager(channelEntityManager), tradeModel(channelEntityManager)
{
  channelEntityManager.registerListener<EDataSubscription>(*this);

  connect(&tradeModel, SIGNAL(rowsAboutToBeRemoved(const QModelIndex&, int, int)), this, SLOT(rowsAboutToBeRemoved(const QModelIndex&, int, int)));

  tradeView = new QTreeView(this);
  tradeView->setUniformRowHeights(true);
  tradeView->setModel(&tradeModel);
  tradeView->setRootIsDecorated(false);
  tradeView->setAlternatingRowColors(true);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->addWidget(tradeView);
  setLayout(layout);

  QHeaderView* headerView = tradeView->header();
  headerView->resizeSection(0, 110);
  headerView->resizeSection(1, 105);
  headerView->resizeSection(2, 105);
  headerView->setStretchLastSection(false);
  headerView->setResizeMode(1, QHeaderView::Stretch);
  settings.beginGroup("LiveTrades");
  settings.beginGroup(channelName);
  headerView->restoreState(settings.value("HeaderState").toByteArray());
  settings.endGroup();
  settings.endGroup();
}

TradesWidget::~TradesWidget()
{
  channelEntityManager.unregisterListener<EDataSubscription>(*this);
}

void TradesWidget::saveState(QSettings& settings)
{
  settings.beginGroup("LiveTrades");
  settings.beginGroup(channelName);
  settings.setValue("HeaderState", tradeView->header()->saveState());
  settings.endGroup();
  settings.endGroup();
}

void TradesWidget::updateTitle()
{
  EDataSubscription* eDataSubscription = channelEntityManager.getEntity<EDataSubscription>(0);
  QString stateStr = eDataSubscription->getStateName();

  QString title;
  if(stateStr.isEmpty())
    title = tr("%1 Live Trades").arg(channelName);
  else
    title = tr("%1 Live Trades (%2)").arg(channelName, stateStr);

  setWindowTitle(title);
  tabFramework.toggleViewAction(this)->setText(tr("Live Trades"));
}

void TradesWidget::rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end)
{
  // ensure the rows to be removed are not selected. otherwise the remove operation will mess up the scroll position.
  QModelIndexList selection = tradeView->selectionModel()->selectedRows();
  for(QModelIndexList::Iterator i = selection.begin(), iend = selection.end(); i != iend; ++i)
  {
    int row = i->row();
    if(row >= start && row <= end)
    {
      tradeView->selectionModel()->clear();
      return;
    }
  }
}

void TradesWidget::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
{
  EDataSubscription* eDataSubscription = dynamic_cast<EDataSubscription*>(&newEntity);
  if(eDataSubscription)
  {
    updateTitle();
    return;
  }
  Q_ASSERT(false);
}
