
#include "stdafx.h"

LoginDialog::LoginDialog(QWidget* parent, QSettings* settings) : QDialog(parent), settings(settings)
{
  setWindowTitle(tr("Login"));

  userEdit = new QLineEdit;
  connect(userEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));

  keyEdit = new QLineEdit;
  connect(keyEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
  keyEdit->setMinimumWidth(200);

  secretEdit = new QLineEdit;
  connect(secretEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));

  marketComboBox = new QComboBox;
  marketComboBox->addItem("Bitstamp/USD");

  rememberComboBox = new QComboBox;
  rememberComboBox->addItem(tr("Nothing")); // 0
  rememberComboBox->addItem(tr("User")); // 1
  rememberComboBox->addItem(tr("User, Key and Secret")); // 2

  QGridLayout *contentLayout = new QGridLayout;
  contentLayout->addWidget(new QLabel(tr("Market:")), 0, 0);
  contentLayout->addWidget(marketComboBox, 0, 1);
  contentLayout->addWidget(new QLabel(tr("User:")), 1, 0);
  contentLayout->addWidget(userEdit, 1, 1);
  contentLayout->addWidget(new QLabel(tr("Key:")), 2, 0);
  contentLayout->addWidget(keyEdit, 2, 1);
  contentLayout->addWidget(new QLabel(tr("Secret:")), 3, 0);
  contentLayout->addWidget(secretEdit, 3, 1);
  contentLayout->addWidget(new QLabel(tr("Remember:")), 4, 0);
  contentLayout->addWidget(rememberComboBox, 4, 1);

  QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
  okButton = buttonBox->button(QDialogButtonBox::Ok);
  okButton->setEnabled(false);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->addLayout(contentLayout);
  layout->addWidget(buttonBox);

  setLayout(layout);

  // load form data
  settings->beginGroup("Login");
  int market = marketComboBox->findText(settings->value("Market").toString());
  if(market >= 0)
    marketComboBox->setCurrentIndex(market);
  userEdit->setText(settings->value("User").toString());
  keyEdit->setText(settings->value("Key").toString());
  secretEdit->setText(settings->value("Secret").toString());
  rememberComboBox->setCurrentIndex(settings->value("Remember", 0).toUInt());
  settings->endGroup();
}

void LoginDialog::showEvent(QShowEvent* event)
{
  QDialog::showEvent(event);

  if(userEdit->text().isEmpty())
    userEdit->setFocus();
  else if(keyEdit->text().isEmpty())
    keyEdit->setFocus();
  else if(secretEdit->text().isEmpty())
    secretEdit->setFocus();
  else
    okButton->setFocus();
}

void LoginDialog::accept()
{
  QDialog::accept();

  // save form data
  settings->beginGroup("Login");
  settings->setValue("Market", marketComboBox->currentText());
  int remeber = rememberComboBox->currentIndex();
  settings->setValue("Remember", remeber);
  settings->setValue("User", remeber >= 1 ? userEdit->text() : QString());
  settings->setValue("Key", remeber >= 2 ? keyEdit->text() : QString());
  settings->setValue("Secret", remeber >= 2 ? secretEdit->text() : QString());
  settings->endGroup();
  settings->sync();
}

void LoginDialog::textChanged()
{
  okButton->setEnabled(!userEdit->text().isEmpty() && !keyEdit->text().isEmpty() && !secretEdit->text().isEmpty());
}
