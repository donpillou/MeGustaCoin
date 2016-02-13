
#include "stdafx.h"

BotPropertiesWidget::BotPropertiesWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager, DataService& dataService) :
  QWidget(&tabFramework), dataService(dataService), propertyModel(entityManager)
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

  class PropertyDelegate : public QStyledItemDelegate
  {
  public:
    PropertyDelegate(QObject* parent) : QStyledItemDelegate(parent) {}
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
      QWidget* widget = QStyledItemDelegate::createEditor(parent, option, index);
      QDoubleSpinBox* spinBox = qobject_cast<QDoubleSpinBox*>(widget);
      if(spinBox)
        spinBox->setDecimals(8);
      return widget;
    }
  };
  propertyView->setItemDelegateForColumn((int)SessionPropertyModel::Column::value, new PropertyDelegate(this));
  propertyView->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::DoubleClicked);
  //itemView->setSelectionMode(QAbstractItemView::ExtendedSelection);

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
  EUserSessionProperty* eProperty = (EUserSessionProperty*)index.internalPointer();
  dataService.updateSessionProperty(*eProperty, value);
}

void BotPropertiesWidget::editedProperty(const QModelIndex& index, double value)
{
  EUserSessionProperty* eProperty = (EUserSessionProperty*)index.internalPointer();
  dataService.updateSessionProperty(*eProperty, QString::number(value));
}

