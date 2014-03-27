
#pragma once

class BotDialog : public QDialog
{
  Q_OBJECT

public:
  BotDialog(QWidget* parent, const QList<QString>& engines);

  QString getName() const {return nameEdit->text();}
  QString getEngine() const {return engineComboBox->currentText();}

private:
  QLineEdit* nameEdit;
  QComboBox* engineComboBox;
  QPushButton* okButton;

  virtual void showEvent(QShowEvent* event);

private slots:
  void textChanged();
};
