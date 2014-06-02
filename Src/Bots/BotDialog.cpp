
#include "stdafx.h"

BotDialog::BotDialog(QWidget* parent, Entity::Manager& entityManager) : QDialog(parent), entityManager(entityManager)
{
  setWindowTitle(tr("Add Bot"));

  nameEdit = new QLineEdit(this);
  connect(nameEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));

  engineComboBox = new QComboBox(this);
  marketComboBox = new QComboBox(this);
  connect(marketComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(marketSelectionChanged(int)));

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
  balanceBaseLabel = new QLabel(tr("Balance:"));
  contentLayout->addWidget(balanceBaseLabel, 3, 0);
  contentLayout->addWidget(balanceBaseSpinBox, 3, 1);
  balanceCommLabel = new QLabel(tr("Balance:"));
  contentLayout->addWidget(balanceCommLabel, 4, 0);
  contentLayout->addWidget(balanceCommSpinBox, 4, 1);

  QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
  okButton = buttonBox->button(QDialogButtonBox::Ok);
  okButton->setEnabled(false);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->addLayout(contentLayout);
  layout->addWidget(buttonBox);

  QList<EBotEngine*> engines;
  QList<EBotMarket*> markets;
  entityManager.getAllEntities<EBotEngine>(engines);
  entityManager.getAllEntities<EBotMarket>(markets);
  foreach(const EBotEngine* engine, engines)
    engineComboBox->addItem(engine->getName(), engine->getId());
  foreach(const EBotMarket* market, markets)
  {
    EBotMarketAdapter* eBotMarketAdapter = entityManager.getEntity<EBotMarketAdapter>(market->getMarketAdapterId());
    if(!eBotMarketAdapter)
      continue;
    marketComboBox->addItem(eBotMarketAdapter->getName(), market->getId());
  }

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
  okButton->setEnabled(!nameEdit->text().isEmpty() && engineComboBox->currentIndex() >= 0 && marketComboBox->currentIndex() >= 0);
}

void BotDialog::marketSelectionChanged(int index)
{
  const EBotMarket* eBotMarket = index >= 0 ? entityManager.getEntity<EBotMarket>(marketComboBox->itemData(index).toUInt()) : 0;
  EBotMarketAdapter* eBotMarketAdapter = eBotMarket ? entityManager.getEntity<EBotMarketAdapter>(eBotMarket->getMarketAdapterId()) : 0;
  balanceBaseLabel->setText(tr("Balance %1:").arg(eBotMarketAdapter ? eBotMarketAdapter->getBaseCurrency() : QString()));
  balanceCommLabel->setText(tr("Balance %1:").arg(eBotMarketAdapter ? eBotMarketAdapter->getCommCurrency() : QString()));
}
