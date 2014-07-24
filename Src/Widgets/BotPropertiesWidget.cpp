
#include "stdafx.h"

BotPropertiesWidget::BotPropertiesWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager, BotService& botService) :
  QWidget(&tabFramework), tabFramework(tabFramework), entityManager(entityManager), botService(botService), propertyModel(entityManager)
{
  setWindowTitle(tr("Bot Properties"));

  propertyView = new QTreeView(this);
  propertyView->setUniformRowHeights(true);
  QSortFilterProxyModel* propertyProxyModel = new QSortFilterProxyModel(this);
  propertyProxyModel->setDynamicSortFilter(true);
  propertyProxyModel->setSourceModel(&propertyModel);
  propertyView->setModel(propertyProxyModel);
  propertyView->setSortingEnabled(true);
  propertyView->setRootIsDecorated(false);
  propertyView->setAlternatingRowColors(true);
  //propertyView->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::DoubleClicked);
  //propertyView->setSelectionMode(QAbstractItemView::ExtendedSelection);

  connect(&propertyModel, SIGNAL(editedProperty(const QModelIndex&, double)), this, SLOT(editedProperty(const QModelIndex&, double)));
  connect(&propertyModel, SIGNAL(editedProperty(const QModelIndex&, const QString&)), this, SLOT(editedProperty(const QModelIndex&, const QString&)));

  QVBoxLayout* layout = new QVBoxLayout;
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->addWidget(propertyView);
  setLayout(layout);

  QHeaderView* headerView = propertyView->header();
  //propertyView->sortByColumn(0);
  settings.beginGroup("BotProperties");
  headerView->restoreState(settings.value("HeaderState").toByteArray());
  settings.endGroup();
  //headerView->setStretchLastSection(false);
  //headerView->setResizeMode(0, QHeaderView::Stretch);
}

void BotPropertiesWidget::saveState(QSettings& settings)
{
  settings.beginGroup("BotProperties");
  settings.setValue("HeaderState", propertyView->header()->saveState());
  settings.endGroup();
}

void BotPropertiesWidget::editedProperty(const QModelIndex& index, const QString& value)
{
  EBotSessionProperty* eProperty = (EBotSessionProperty*)index.internalPointer();
  botService.updateSessionProperty(*eProperty, value);
}

void BotPropertiesWidget::editedProperty(const QModelIndex& index, double value)
{
  EBotSessionProperty* eProperty = (EBotSessionProperty*)index.internalPointer();
  botService.updateSessionProperty(*eProperty, QString::number(value));
}

