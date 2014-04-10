
#pragma once

class BotDialog : public QDialog
{
  Q_OBJECT

public:
  BotDialog(QWidget* parent, const QList<EBotEngine*>& engines, const QString& currencyBase, const QString& currencyComm);

  QString getName() const {return nameEdit->text();}
  QString getEngine() const {return engineComboBox->currentText();}
  double getBalanceBase() const {return balanceBaseSpinBox->value();}
  double getBalanceComm() const {return balanceCommSpinBox->value();}

private:
  QLineEdit* nameEdit;
  QComboBox* engineComboBox;
  QDoubleSpinBox * balanceBaseSpinBox;
  QDoubleSpinBox * balanceCommSpinBox;
  QPushButton* okButton;

  virtual void showEvent(QShowEvent* event);

private slots:
  void textChanged();
};
