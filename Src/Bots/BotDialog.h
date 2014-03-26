
#pragma once

class BotDialog : public QDialog
{
  Q_OBJECT

public:
  BotDialog(QWidget* parent);

private:
  QLineEdit* nameEdit;
  QComboBox* engineComboBox;
  QPushButton* okButton;

  virtual void showEvent(QShowEvent* event);

private slots:
  void textChanged();
};
