
#include "stdafx.h"

BotDialog::BotDialog(QWidget* parent, const QList<EBotEngine*>& engines, const QString& currencyBase, const QString& currencyComm) : QDialog(parent)
{
  setWindowTitle(tr("Add Bot"));

  nameEdit = new QLineEdit(this);
  connect(nameEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));

  engineComboBox = new QComboBox(this);
  foreach(const EBotEngine* engine, engines)
    engineComboBox->addItem(engine->getName());

  balanceBaseSpinBox = new QDoubleSpinBox(this);
  balanceBaseSpinBox->setValue(100.);
  balanceBaseSpinBox->setMinimum(0.);
  balanceBaseSpinBox->setMaximum(9999999.);
  balanceCommSpinBox = new QDoubleSpinBox(this);
  balanceCommSpinBox->setValue(0.);
  balanceCommSpinBox->setMinimum(0.);
  balanceCommSpinBox->setMaximum(9999999.);

  QGridLayout *contentLayout = new QGridLayout;
  contentLayout->addWidget(new QLabel(tr("Name:")), 0, 0);
  contentLayout->addWidget(nameEdit, 0, 1);
  contentLayout->addWidget(new QLabel(tr("Engine:")), 1, 0);
  contentLayout->addWidget(engineComboBox, 1, 1);
  contentLayout->addWidget(new QLabel(tr("Balance %1:").arg(currencyBase)), 2, 0);
  contentLayout->addWidget(balanceBaseSpinBox, 2, 1);
  contentLayout->addWidget(new QLabel(tr("Balance %1:").arg(currencyComm)), 3, 0);
  contentLayout->addWidget(balanceCommSpinBox, 3, 1);

  QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
  okButton = buttonBox->button(QDialogButtonBox::Ok);
  okButton->setEnabled(false);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->addLayout(contentLayout);
  layout->addWidget(buttonBox);

  setLayout(layout);
}

void BotDialog::showEvent(QShowEvent* event)
{
  QDialog::showEvent(event);

  //nameEdit->setFocus();
}

void BotDialog::textChanged()
{
  okButton->setEnabled(!nameEdit->text().isEmpty());
}
