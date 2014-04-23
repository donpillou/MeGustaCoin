
#include "stdafx.h"

BotDialog::BotDialog(QWidget* parent, Entity::Manager& entityManager) : QDialog(parent)
{
  setWindowTitle(tr("Add Bot"));

  nameEdit = new QLineEdit(this);
  connect(nameEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));

  QList<EBotEngine*> engines;
  QList<EBotMarketAdapter*> marketAdapters;
  entityManager.getAllEntities<EBotEngine>(engines);
  entityManager.getAllEntities<EBotMarketAdapter>(marketAdapters);

  engineComboBox = new QComboBox(this);
  foreach(const EBotEngine* engine, engines)
    engineComboBox->addItem(engine->getName(), engine->getId());
  marketComboBox = new QComboBox(this);
  foreach(const EBotMarketAdapter* marketAdapter, marketAdapters)
    marketComboBox->addItem(marketAdapter->getName(), marketAdapter->getId());

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
  contentLayout->addWidget(new QLabel(tr("Market:")), 2, 0);
  contentLayout->addWidget(marketComboBox, 2, 1);
  int botMarketIndex = marketComboBox->currentIndex();
  const EBotMarketAdapter* eBotMarket = botMarketIndex >= 0 ? entityManager.getEntity<EBotMarketAdapter>(marketComboBox->itemData(botMarketIndex).toUInt()) : 0;
  contentLayout->addWidget(new QLabel(tr("Balance %1:").arg(eBotMarket ? eBotMarket->getBaseCurrency() : QString())), 3, 0);
  contentLayout->addWidget(balanceBaseSpinBox, 3, 1);
  contentLayout->addWidget(new QLabel(tr("Balance %1:").arg(eBotMarket ? eBotMarket->getCommCurrency() : QString())), 4, 0);
  contentLayout->addWidget(balanceCommSpinBox, 4, 1);

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

quint32 BotDialog::getEngineId() const
{
  int index = engineComboBox->currentIndex();
  if(index >= 0)
    return engineComboBox->itemData(index).toUInt();
  return 0;
}

quint32 BotDialog::getMarketId() const
{
  int index = marketComboBox->currentIndex();
  if(index >= 0)
    return marketComboBox->itemData(index).toUInt();
  return 0;
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
