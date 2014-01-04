
#include "stdafx.h"

GraphWidget::GraphWidget(QWidget* parent, QSettings& settings, const PublicDataModel* publicDataModel, const QString& settingsSection, const QMap<QString, PublicDataModel*>& publicDataModels) :
  QWidget(parent), publicDataModel(publicDataModel), settingsSection(settingsSection),
  publicDataModels(publicDataModels)
{
  if(publicDataModel)
    connect(publicDataModel, SIGNAL(changedState()), this, SLOT(updateTitle()));

  /*
  setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
  setBackgroundRole(QPalette::Base);
  setAutoFillBackground(true);
  */
  QToolBar* toolBar = new QToolBar(this);
  toolBar->setStyleSheet("QToolBar { border: 0px }");
  toolBar->setIconSize(QSize(16, 16));
  toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

  zoomAction = toolBar->addAction(QIcon(":/Icons/zoom.png"), tr("1 Hour"));
  //zoomAction->setEnabled(false);
  QMenu* zoomMenu = new QMenu(this);
  QActionGroup* zoomActionGroup = new QActionGroup(zoomMenu);
  zoomSignalMapper = new QSignalMapper(zoomMenu);
  //connect(zoomAction, SIGNAL(triggered()), zoomSignalMapper, SLOT(map()));
  connect(zoomSignalMapper, SIGNAL(mapped(int)), SLOT(setZoom(int)));
  QAction* action = zoomMenu->addAction(tr("10 Minutes"));
  action->setCheckable(true);
  zoomActionGroup->addAction(action);
  zoomSignalMapper->setMapping(action, 10 * 60);
  connect(action, SIGNAL(triggered()), zoomSignalMapper, SLOT(map()));
  action = zoomMenu->addAction(tr("30 Minutes"));
  action->setCheckable(true);
  zoomActionGroup->addAction(action);
  zoomSignalMapper->setMapping(action, 30 * 60);
  connect(action, SIGNAL(triggered()), zoomSignalMapper, SLOT(map()));
  action = zoomMenu->addAction(tr("1 Hour"));
  action->setCheckable(true);
  zoomActionGroup->addAction(action);
  zoomSignalMapper->setMapping(action, 60 * 60);
  connect(action, SIGNAL(triggered()), zoomSignalMapper, SLOT(map()));
  action = zoomMenu->addAction(tr("2 Hour"));
  action->setCheckable(true);
  zoomActionGroup->addAction(action);
  zoomSignalMapper->setMapping(action, 2 * 60 * 60);
  connect(action, SIGNAL(triggered()), zoomSignalMapper, SLOT(map()));
  action = zoomMenu->addAction(tr("3 Hours"));
  action->setCheckable(true);
  zoomActionGroup->addAction(action);
  zoomSignalMapper->setMapping(action, 3 * 60 * 60);
  connect(action, SIGNAL(triggered()), zoomSignalMapper, SLOT(map()));
  action = zoomMenu->addAction(tr("6 Hours"));
  action->setCheckable(true);
  zoomActionGroup->addAction(action);
  zoomSignalMapper->setMapping(action, 6 * 60 * 60);
  connect(action, SIGNAL(triggered()), zoomSignalMapper, SLOT(map()));
  action = zoomMenu->addAction(tr("12 Hours"));
  action->setCheckable(true);
  zoomActionGroup->addAction(action);
  zoomSignalMapper->setMapping(action, 12 * 60 * 60);
  connect(action, SIGNAL(triggered()), zoomSignalMapper, SLOT(map()));
  action = zoomMenu->addAction(tr("24 Hours"));
  action->setCheckable(true);
  zoomActionGroup->addAction(action);
  zoomSignalMapper->setMapping(action, 24 * 60 * 60);
  connect(action, SIGNAL(triggered()), zoomSignalMapper, SLOT(map()));
  action = zoomMenu->addAction(tr("2 Days"));
  action->setCheckable(true);
  zoomActionGroup->addAction(action);
  zoomSignalMapper->setMapping(action, 2 * 24 * 60 * 60);
  connect(action, SIGNAL(triggered()), zoomSignalMapper, SLOT(map()));
  action = zoomMenu->addAction(tr("3 Days"));
  action->setCheckable(true);
  zoomActionGroup->addAction(action);
  zoomSignalMapper->setMapping(action, 3 * 24 * 60 * 60);
  connect(action, SIGNAL(triggered()), zoomSignalMapper, SLOT(map()));
  zoomAction->setMenu(zoomMenu);  
  qobject_cast<QToolButton*>(toolBar->widgetForAction(zoomAction))->setPopupMode(QToolButton::InstantPopup);
  
  QAction* dataAction = toolBar->addAction(QIcon(":/Icons/chart_curve.png"), tr("Data"));
  dataMenu = new QMenu(this);
  connect(dataMenu, SIGNAL(aboutToShow()), this, SLOT(updateDataMenu()));
  dataSignalMapper = new QSignalMapper(dataMenu);
  connect(dataSignalMapper, SIGNAL(mapped(int)), SLOT(setEnabledData(int)));
  dataAction->setMenu(dataMenu);
  qobject_cast<QToolButton*>(toolBar->widgetForAction(dataAction))->setPopupMode(QToolButton::InstantPopup);
  
  QFrame* frame = new QFrame(this);
  frame->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
  frame->setBackgroundRole(QPalette::Base);
  frame->setAutoFillBackground(true);

  graphView = new GraphView(this, publicDataModel, publicDataModels);
  QVBoxLayout* graphLayout = new QVBoxLayout;
  graphLayout->setMargin(0);
  graphLayout->setSpacing(0);
  graphLayout->addWidget(graphView);
  frame->setLayout(graphLayout);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->addWidget(toolBar);
  layout->addWidget(frame);
  setLayout(layout);

  settings.beginGroup("LiveGraph");
  settings.beginGroup(settingsSection);
  action = qobject_cast<QAction*>(zoomSignalMapper->mapping(settings.value("Zoom", 60 * 60).toUInt()));
  action->setChecked(true);
  zoomSignalMapper->map(action);
  unsigned int enabledData = settings.value("EnabledData", (unsigned int)graphView->getEnabledData()).toUInt();
  graphView->setEnabledData(enabledData);
  settings.endGroup();
  settings.endGroup();
}

void GraphWidget::saveState(QSettings& settings)
{
  settings.beginGroup("LiveGraph");
  settings.beginGroup(settingsSection);
  settings.setValue("Zoom", graphView->getMaxAge());
  settings.setValue("EnabledData", graphView->getEnabledData());
  settings.endGroup();
  settings.endGroup();
}

void GraphWidget::setFocusPublicDataModel(const PublicDataModel* publicDataModel)
{
  if(this->publicDataModel)
    disconnect(this->publicDataModel, SIGNAL(changedState()), this, SLOT(updateTitle()));
  this->publicDataModel = publicDataModel;
  if(publicDataModel)
    connect(publicDataModel, SIGNAL(changedState()), this, SLOT(updateTitle()));

  graphView->setFocusPublicDataModel(publicDataModel);
  updateTitle();
}

void GraphWidget::setZoom(int maxTime)
{
  QAction* srcAction = qobject_cast<QAction*>(zoomSignalMapper->mapping(maxTime));
  zoomAction->setText(srcAction->text());
  graphView->setMaxAge(maxTime);
  graphView->update();
}

void GraphWidget::setEnabledData(int data)
{
  unsigned int enabledData = graphView->getEnabledData();
  bool enable = (enabledData & data) == 0;
  if(enable)
    graphView->setEnabledData(enabledData | data);
  else
    graphView->setEnabledData(enabledData & ~data);
  graphView->update();
}

void GraphWidget::updateDataMenu()
{
  foreach(QAction* action, dataMenu->actions())
    dataSignalMapper->removeMappings(action);
  dataMenu->clear();

  if(!publicDataModel)
    return;

  QAction* action = dataMenu->addAction(tr("Trades"));
  action->setCheckable(true);
  dataSignalMapper->setMapping(action, (int)GraphView::Data::trades);
  connect(action, SIGNAL(triggered()), dataSignalMapper, SLOT(map()));
  action = dataMenu->addAction(tr("Trade Volume"));
  action->setCheckable(true);
  dataSignalMapper->setMapping(action, (int)GraphView::Data::tradeVolume);
  connect(action, SIGNAL(triggered()), dataSignalMapper, SLOT(map()));
  if(publicDataModel->getFeatures() & (int)MarketStream::Features::orderBook)
  {
    action = dataMenu->addAction(tr("Order Book"));
    action->setCheckable(true);
    dataSignalMapper->setMapping(action, (int)GraphView::Data::orderBook);
    connect(action, SIGNAL(triggered()), dataSignalMapper, SLOT(map()));
  }
  action = dataMenu->addAction(tr("Regression Lines"));
  action->setCheckable(true);
  dataSignalMapper->setMapping(action, (int)GraphView::Data::regressionLines);
  connect(action, SIGNAL(triggered()), dataSignalMapper, SLOT(map()));
  action = dataMenu->addAction(tr("Other Markets"));
  action->setCheckable(true);
  dataSignalMapper->setMapping(action, (int)GraphView::Data::otherMarkets);
  connect(action, SIGNAL(triggered()), dataSignalMapper, SLOT(map()));

  unsigned int enabledData = graphView->getEnabledData();
  for(int i = 0; i < 16; ++i)
    if(enabledData & (1 << i))
    {
      QObject* obj = dataSignalMapper->mapping(1 << i);
      if(obj)
      {
        action = qobject_cast<QAction*>(obj);
        action->setChecked(true);
      }
    }
}

void GraphWidget::updateTitle()
{
  QString stateStr;
  if(publicDataModel)
    stateStr = publicDataModel->getStateName();

  QString title;
  if(publicDataModel)
  {
    if(stateStr.isEmpty())
      title = tr("%1 Live Graph").arg(publicDataModel->getMarketName());
    else
      title = tr("%1 Live Graph (%2)").arg(publicDataModel->getMarketName(), stateStr);
  }
  else
    title = tr("Live Graph (%1)").arg(tr("offline"));

  QDockWidget* dockWidget = qobject_cast<QDockWidget*>(parent());
  dockWidget->setWindowTitle(title);
  dockWidget->toggleViewAction()->setText(tr("Live Graph"));
}
