
#include "stdafx.h"

MainWindow::MainWindow() : settings(QSettings::IniFormat, QSettings::UserScope, "MeGustaCoin", "MeGustaCoin"), market(0)
{
  orderView = new QTreeView(this);
  orderProxyModel = new QSortFilterProxyModel(this);
  orderProxyModel->setDynamicSortFilter(true);
  orderView->setModel(orderProxyModel);
  //orderView->setIndentation(0);
  orderView->setSortingEnabled(true);
  orderView->setRootIsDecorated(false);
  orderView->setAlternatingRowColors(true);

  setWindowTitle(tr("MeGustaCoin Trading Client"));
  setCentralWidget(orderView);
  resize(600, 400);
  //statusBar()->showMessage(tr("Ready"));

  QMenuBar* menuBar = this->menuBar();
  QMenu* menu = menuBar->addMenu(tr("&Market"));
  QAction* action = menu->addAction(tr("&Login..."));
  action->setShortcut(QKeySequence(QKeySequence::Open));
  connect(action, SIGNAL(triggered()), this, SLOT(login()));
  action = menu->addAction(tr("Log&out"));
  action->setShortcut(QKeySequence(QKeySequence::Close));
  connect(action, SIGNAL(triggered()), this, SLOT(logout()));
  menu->addSeparator();
  action = menu->addAction(tr("&Exit"));
  action->setShortcut(QKeySequence::Quit);
  connect(action, SIGNAL(triggered()), this, SLOT(close()));

  menu = menuBar->addMenu(tr("&View"));
  action = menu->addAction(tr("&Refresh"));
  action->setShortcut(QKeySequence(QKeySequence::Refresh));
  connect(action, SIGNAL(triggered()), this, SLOT(refresh()));

  restoreGeometry(settings.value("Geometry").toByteArray());
  restoreState(settings.value("WindowState").toByteArray());

  settings.beginGroup("Login");
  if(settings.value("Remember", 0).toUInt() >= 2)
  {
    QString market = settings.value("Market").toString();
    QString user = settings.value("User").toString();
    QString key = settings.value("Key").toString();
    QString secret = settings.value("Secret").toString();
    open(market, user, key, secret);
  }
  settings.endGroup();

  if(!market)
    QTimer::singleShot(0, this, SLOT(login()));
}

MainWindow::~MainWindow()
{
  logout();
}

void MainWindow::login()
{
  LoginDialog loginDialog(this, &settings);
  if(loginDialog.exec() != QDialog::Accepted)
    return;

  open(loginDialog.market(), loginDialog.userName(), loginDialog.key(), loginDialog.secret());
}

void MainWindow::logout()
{
  if(!market)
    return;

  settings.setValue("Geometry", saveGeometry());
  settings.setValue("WindowState", saveState());
  settings.setValue("OrderHeaderState", orderView->header()->saveState());
  //settings.setValue("OrderHeaderGeometry", orderView->header()->saveGeometry());

  //orderView->setModel(0);
  orderProxyModel->setSourceModel(0);
  delete market;
  market = 0;
}

void MainWindow::refresh()
{
  if(!market)
    return;
  market->loadOrders();
}

void MainWindow::open(const QString& marketName, const QString& userName, const QString& key, const QString& secret)
{
  logout();

  // login
  if(marketName == "Bitstamp/USD")
  {
    market = new BitstampMarket(userName, key, secret);
  }
  if(!market)
    return;

  // update gui
  orderProxyModel->setSourceModel(&market->getOrderModel());
  orderView->header()->resizeSection(0, 85);
  orderView->header()->resizeSection(1, 85);
  orderView->header()->resizeSection(2, 150);
  orderView->header()->resizeSection(3, 85);
  orderView->header()->resizeSection(4, 85);
  orderView->header()->resizeSection(5, 85);
  //orderView->header()->restoreState(settings.value("OrderHeaderState").toByteArray());
  //orderView->setModel(&market->getOrderModel());
  setWindowTitle(userName + "@" + marketName);
  updateStatusBar();

  // request data
  refresh();
}

void MainWindow::updateStatusBar()
{
}

void MainWindow::closeEvent(QCloseEvent* event)
{
  logout();

  QMainWindow::closeEvent(event);
}