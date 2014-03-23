
#include "stdafx.h"

OptionsDialog::OptionsDialog(QWidget* parent, QSettings* settings) : QDialog(parent), settings(settings)
{
  setWindowTitle(tr("Options"));

  dataServerEdit = new QLineEdit;

  botServerEdit = new QLineEdit;
  botUserEdit = new QLineEdit;
  botUserEdit->setEchoMode(QLineEdit::PasswordEchoOnEdit);
  botPasswordEdit = new QLineEdit;
  botPasswordEdit->setEchoMode(QLineEdit::Password);

  QGridLayout* dataLayout = new QGridLayout;
  dataLayout->addWidget(new QLabel(tr("Address:")), 0, 0);
  dataLayout->addWidget(dataServerEdit, 0, 1);

  QGridLayout* botLayout = new QGridLayout;
  botLayout->addWidget(new QLabel(tr("Address:")), 0, 0);
  botLayout->addWidget(botServerEdit, 0, 1);
  botLayout->addWidget(new QLabel(tr("User:")), 1, 0);
  botLayout->addWidget(botUserEdit, 1, 1);
  botLayout->addWidget(new QLabel(tr("Password:")), 2, 0);
  botLayout->addWidget(botPasswordEdit, 2, 1);

  QGroupBox* dataGroupBox = new QGroupBox(tr("Data Server (traded)"));
  dataGroupBox->setLayout(dataLayout);

  QGroupBox* botGroupBox = new QGroupBox(tr("Bot Server (tradedbot)"));
  botGroupBox->setLayout(botLayout);

  QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
  okButton = buttonBox->button(QDialogButtonBox::Ok);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->addWidget(dataGroupBox);
  layout->addWidget(botGroupBox);
  layout->addWidget(buttonBox);

  setLayout(layout);

  // load form data
  settings->beginGroup("DataServer");
  dataServerEdit->setText(settings->value("Address", "127.0.0.1:40123").toString());
  settings->endGroup();
  settings->beginGroup("BotServer");
  botServerEdit->setText(settings->value("Address", "127.0.0.1:40124").toString());
  botUserEdit->setText(settings->value("User").toString());
  botPasswordEdit->setText(settings->value("Password").toString());
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
  settings->endGroup();
  settings->beginGroup("BotServer");
  settings->setValue("Address", botServerEdit->text());
  settings->setValue("User", botUserEdit->text());
  settings->setValue("Password", botPasswordEdit->text());
  settings->endGroup();
  settings->sync();
}
