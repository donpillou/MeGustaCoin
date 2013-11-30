
#pragma once

class LoginDialog : public QDialog
{
  Q_OBJECT

public:
  LoginDialog(QWidget* parent, QSettings* settings);

  QString market() const {return marketComboBox->currentText();}
  QString userName() const {return userEdit->text();}
  QString key() const {return keyEdit->text();}
  QString secret() const {return secretEdit->text();}

private:
  QSettings* settings;
  QLineEdit* userEdit;
  QLineEdit* keyEdit;
  QLineEdit* secretEdit;
  QComboBox* marketComboBox;
  QComboBox* rememberComboBox;
  QPushButton* okButton;

  virtual void showEvent(QShowEvent* event);
  virtual void accept();

private slots:
  void textChanged();
};
