
#include "stdafx.h"

BotLogWidget::BotLogWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager) :
  QWidget(&tabFramework), tabFramework(tabFramework), entityManager(entityManager),  logModel(entityManager), autoScrollEnabled(false)
{
  entityManager.registerListener<EBotService>(*this);

  connect(&logModel, SIGNAL(rowsAboutToBeInserted(const QModelIndex&, int, int)), this, SLOT(checkAutoScroll(const QModelIndex&, int, int)));

  logView = new QTreeView(this);
  connect(logView->verticalScrollBar(), SIGNAL(rangeChanged(int, int)), this, SLOT(autoScroll(int, int)));
  logView->setUniformRowHeights(true);
  logView->setModel(&logModel);
  logView->setRootIsDecorated(false);
  logView->setAlternatingRowColors(true);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->addWidget(logView);
  setLayout(layout);

  QHeaderView* headerView = logView->header();
  headerView->resizeSection(0, 110);
  headerView->resizeSection(1, 200);
  logView->sortByColumn(0, Qt::AscendingOrder);
  settings.beginGroup("BotLog");
  headerView->restoreState(settings.value("HeaderState").toByteArray());
  settings.endGroup();
}

BotLogWidget::~BotLogWidget()
{
  entityManager.unregisterListener<EBotService>(*this);
}

void BotLogWidget::saveState(QSettings& settings)
{
  settings.beginGroup("BotLog");
  settings.setValue("HeaderState", logView->header()->saveState());
  settings.endGroup();
}

void BotLogWidget::updateTitle(EBotService& eBotService)
{
  QString stateStr = eBotService.getStateName();

  QString title;
  if(stateStr.isEmpty())
    title = tr("Bot Log");
  else
    title = tr("Bot Log (%1)").arg(stateStr);

  setWindowTitle(title);
  tabFramework.toggleViewAction(this)->setText(tr("Bot Log"));
}

void BotLogWidget::checkAutoScroll(const QModelIndex& index, int, int)
{
  QScrollBar* scrollBar = logView->verticalScrollBar();
  if(scrollBar->value() == scrollBar->maximum())
    autoScrollEnabled = true;
}

void BotLogWidget::autoScroll(int, int)
{
  if(!autoScrollEnabled)
    return;
  QScrollBar* scrollBar = logView->verticalScrollBar();
  scrollBar->setValue(scrollBar->maximum());
  autoScrollEnabled = false;
}

void BotLogWidget::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
{
  EBotService* eBotService = dynamic_cast<EBotService*>(&newEntity);
  if(eBotService)
  {
    updateTitle(*eBotService);
    return;
  }
  Q_ASSERT(false);
}
