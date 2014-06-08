
#include "stdafx.h"

TradesWidget::TradesWidget(QWidget* parent, QSettings& settings, const QString& channelName, Entity::Manager& entityManager) :
  QWidget(parent),
  channelName(channelName), entityManager(entityManager), tradeModel(entityManager)//, autoScrollEnabled(false)
{
  // todo: update title

  tradeView = new QTreeView(this);
  //connect(tradeView->verticalScrollBar(), SIGNAL(rangeChanged(int, int)), this, SLOT(autoScroll(int, int)));
  tradeView->setUniformRowHeights(true);
  tradeView->setModel(&tradeModel);
  //tradeView->setSortingEnabled(true);
  tradeView->setRootIsDecorated(false);
  tradeView->setAlternatingRowColors(true);
  //tradeView->setSelectionMode(QAbstractItemView::ExtendedSelection);

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
  EDataSubscription* eDataSubscription = entityManager.getEntity<EDataSubscription>(0);
  QString stateStr = eDataSubscription->getStateName();

  QString title;
  if(stateStr.isEmpty())
    title = tr("%1 Live Trades").arg(channelName);
  else
    title = tr("%1 Live Trades (%2)").arg(channelName, stateStr);

  QDockWidget* dockWidget = qobject_cast<QDockWidget*>(parent());
  dockWidget->setWindowTitle(title);
  dockWidget->toggleViewAction()->setText(tr("Live Trades"));
}
