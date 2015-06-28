
#include "stdafx.h"

OptionsDialog::OptionsDialog(QWidget* parent, QSettings* settings) : QDialog(parent), settings(settings)
{
  setWindowTitle(tr("Options"));

  dataServerEdit = new QLineEdit;
  dataUserEdit = new QLineEdit;
  dataUserEdit->setEchoMode(QLineEdit::PasswordEchoOnEdit);
  dataPasswordEdit = new QLineEdit;
  dataPasswordEdit->setEchoMode(QLineEdit::Password);


  QGridLayout* dataLayout = new QGridLayout;
  dataLayout->addWidget(new QLabel(tr("Address:")), 0, 0);
  dataLayout->addWidget(dataServerEdit, 0, 1);
  dataLayout->addWidget(new QLabel(tr("User:")), 1, 0);
  dataLayout->addWidget(dataUserEdit, 1, 1);
  dataLayout->addWidget(new QLabel(tr("Password:")), 2, 0);
  dataLayout->addWidget(dataPasswordEdit, 2, 1);

  QGroupBox* dataGroupBox = new QGroupBox(tr("ZlimDB Server"));
  dataGroupBox->setLayout(dataLayout);

  QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
  okButton = buttonBox->button(QDialogButtonBox::Ok);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->addWidget(dataGroupBox);
  layout->addWidget(buttonBox);

  setLayout(layout);

  // load form data
  settings->beginGroup("DataServer");
  dataServerEdit->setText(settings->value("Address", "127.0.0.1:13211").toString());
  dataUserEdit->setText(settings->value("User").toString());
  dataPasswordEdit->setText(settings->value("Password").toString());
  settings->endGroup();
}

void OptionsDialog::showEvent(QShowEvent* event)
{
  QDialog::showEvent(event);

  okButton->setFocus();
}

void OptionsDialog::accept()
{
  QDialog::accept();

  // save form data
  settings->beginGroup("DataServer");
  settings->setValue("Address", dataServerEdit->text());
  settings->setValue("User", dataUserEdit->text());
  settings->setValue("Password", dataPasswordEdit->text());
  settings->endGroup();
  settings->sync();
}
