
#include "stdafx.h"

MarketDialog::MarketDialog(QWidget* parent, Entity::Manager& entityManager) : QDialog(parent)
{
  setWindowTitle(tr("Add Market"));

  QList<EBotMarketAdapter*> marketAdapters;
  entityManager.getAllEntities<EBotMarketAdapter>(marketAdapters);

  marketComboBox = new QComboBox(this);
  foreach(const EBotMarketAdapter* marketAdapter, marketAdapters)
    marketComboBox->addItem(marketAdapter->getName(), marketAdapter->getId());

  userEdit = new QLineEdit;
  connect(userEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
  userEdit->setEchoMode(QLineEdit::PasswordEchoOnEdit);

  keyEdit = new QLineEdit;
  connect(keyEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
  keyEdit->setMinimumWidth(240);
  keyEdit->setEchoMode(QLineEdit::PasswordEchoOnEdit);

  secretEdit = new QLineEdit;
  connect(secretEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
  secretEdit->setEchoMode(QLineEdit::Password);

  QGridLayout *contentLayout = new QGridLayout;
  contentLayout->addWidget(new QLabel(tr("Market:")), 0, 0);
  contentLayout->addWidget(marketComboBox, 0, 1);
  int botMarketIndex = marketComboBox->currentIndex();
  //const EBotMarketAdapter* eBotMarket = botMarketIndex >= 0 ? entityManager.getEntity<EBotMarketAdapter>(marketComboBox->itemData(botMarketIndex).toUInt()) : 0;
  contentLayout->addWidget(new QLabel(tr("User:")), 1, 0);
  contentLayout->addWidget(userEdit, 1, 1);
  contentLayout->addWidget(new QLabel(tr("Key:")), 2, 0);
  contentLayout->addWidget(keyEdit, 2, 1);
  contentLayout->addWidget(new QLabel(tr("Secret:")), 3, 0);
  contentLayout->addWidget(secretEdit, 3, 1);

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

quint32 MarketDialog::getMarketAdapterId() const
{
  int index = marketComboBox->currentIndex();
  if(index >= 0)
    return marketComboBox->itemData(index).toUInt();
  return 0;
}

void MarketDialog::showEvent(QShowEvent* event)
{
  QDialog::showEvent(event);

  marketComboBox->setFocus();
}

void MarketDialog::textChanged()
{
  okButton->setEnabled(!userEdit->text().isEmpty() && !keyEdit->text().isEmpty() && !secretEdit->text().isEmpty());
}
