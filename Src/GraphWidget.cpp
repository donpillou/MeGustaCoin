
#include "stdafx.h"

GraphWidget::GraphWidget(QWidget* parent, QSettings& settings, DataModel& dataModel) : QWidget(parent), dataModel(dataModel), graphModel(dataModel.graphModel), zoom(60 * 60)
{
  /*
  setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
  setBackgroundRole(QPalette::Base);
  setAutoFillBackground(true);
  */
  QToolBar* toolBar = new QToolBar(this);
  toolBar->setStyleSheet("QToolBar { border: 0px }");
  toolBar->setIconSize(QSize(16, 16));
  toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

  zoomAction = toolBar->addAction(tr("1 Hour"));
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
  action = zoomMenu->addAction(tr("1 Hour"));
  action->setCheckable(true);
  zoomActionGroup->addAction(action);
  zoomSignalMapper->setMapping(action, 60 * 60);
  connect(action, SIGNAL(triggered()), zoomSignalMapper, SLOT(map()));
  action = zoomMenu->addAction(tr("3 Hours"));
  action->setCheckable(true);
  zoomActionGroup->addAction(action);
  zoomSignalMapper->setMapping(action, 3 * 60 * 60);
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
  action = zoomMenu->addAction(tr("3 Days"));
  action->setCheckable(true);
  zoomActionGroup->addAction(action);
  zoomSignalMapper->setMapping(action, 3 * 24 * 60 * 60);
  connect(action, SIGNAL(triggered()), zoomSignalMapper, SLOT(map()));
  zoomAction->setMenu(zoomMenu);  
  qobject_cast<QToolButton*>(toolBar->widgetForAction(zoomAction))->setPopupMode(QToolButton::InstantPopup);
  
  QAction* dataAction = toolBar->addAction(tr("Data"));
  QMenu* dataMenu = new QMenu(this);
  QSignalMapper* dataSignalMapper = new QSignalMapper(dataMenu);
  connect(dataSignalMapper, SIGNAL(mapped(int)), SLOT(setEnabledData(int)));
  action = dataMenu->addAction(tr("Trades"));
  action->setCheckable(true);
  dataSignalMapper->setMapping(action, (int)GraphView::Data::trades);
  connect(action, SIGNAL(triggered()), dataSignalMapper, SLOT(map()));
  action = dataMenu->addAction(tr("Trade Volume"));
  action->setCheckable(true);
  dataSignalMapper->setMapping(action, (int)GraphView::Data::tradeVolume);
  connect(action, SIGNAL(triggered()), dataSignalMapper, SLOT(map()));
  action = dataMenu->addAction(tr("Order Book"));
  action->setCheckable(true);
  dataSignalMapper->setMapping(action, (int)GraphView::Data::orderBook);
  connect(action, SIGNAL(triggered()), dataSignalMapper, SLOT(map()));
  dataAction->setMenu(dataMenu);
  qobject_cast<QToolButton*>(toolBar->widgetForAction(dataAction))->setPopupMode(QToolButton::InstantPopup);
  
  QFrame* frame = new QFrame(this);
  frame->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
  frame->setBackgroundRole(QPalette::Base);
  frame->setAutoFillBackground(true);

  graphView = new GraphView(this, graphModel);
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
  action = qobject_cast<QAction*>(zoomSignalMapper->mapping(settings.value("Zoom", 60 * 60).toUInt()));
  action->setChecked(true);
  zoomSignalMapper->map(action);
  unsigned int enabledData = settings.value("EnabledData", (unsigned int)graphView->getEnabledData()).toUInt();
  graphView->setEnabledData(enabledData);
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
  settings.endGroup();
}

void GraphWidget::saveState(QSettings& settings)
{
  settings.beginGroup("LiveGraph");
  settings.setValue("Zoom", zoom);
  settings.setValue("EnabledData", graphView->getEnabledData());
  settings.endGroup();
}

void GraphWidget::setMarket(Market* market)
{
  this->market = market;
  graphView->setMarket(market);
}

void GraphWidget::setZoom(int maxTime)
{
  this->zoom = maxTime;
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
