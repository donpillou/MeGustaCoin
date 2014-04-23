
#include "stdafx.h"

MarketsDialog::MarketsDialog(QWidget* parent, QSettings& settings, Entity::Manager& entityManager) : QDialog(parent), settings(settings),
  botMarketModel(entityManager)
{
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

  QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
  okButton = buttonBox->button(QDialogButtonBox::Ok);
  okButton->setEnabled(false);

  QVBoxLayout* contentLayout = new QVBoxLayout;
  contentLayout->setMargin(0);
  contentLayout->setSpacing(0);
  contentLayout->addWidget(toolBar);
  contentLayout->addWidget(marketView);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->addLayout(contentLayout);
  layout->addWidget(buttonBox);

  setLayout(layout);

  // load form data
  //settings->beginGroup("Login");
  //int market = marketComboBox->findText(settings->value("Market").toString());
  //if(market >= 0)
  //  marketComboBox->setCurrentIndex(market);
  //userEdit->setText(settings->value("User").toString());
  //keyEdit->setText(settings->value("Key").toString());
  //secretEdit->setText(settings->value("Secret").toString());
  //unsigned int remember = settings->value("Remember", 0).toUInt();
  //rememberComboBox->setCurrentIndex(remember);
  //settings->endGroup();
}

void MarketsDialog::showEvent(QShowEvent* event)
{
  QDialog::showEvent(event);

  //okButton->setFocus();
}

void MarketsDialog::accept()
{
  QDialog::accept();

  // save form data
  //settings->beginGroup("Login");
  //settings->setValue("Market", marketComboBox->currentText());
  //int remeber = rememberComboBox->currentIndex();
  //settings->setValue("Remember", remeber);
  //settings->setValue("User", remeber >= 1 ? userEdit->text() : QString());
  //settings->setValue("Key", remeber >= 2 ? keyEdit->text() : QString());
  //settings->setValue("Secret", remeber >= 2 ? secretEdit->text() : QString());
  //settings->endGroup();
  //settings->sync();
}

void MarketsDialog::addMarket()
{
}

void MarketsDialog::editMarket()
{
}

void MarketsDialog::removeMarket()
{
}
