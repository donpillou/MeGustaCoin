
#pragma once

class OptionsDialog : public QDialog
{
  Q_OBJECT

public:
  OptionsDialog(QWidget* parent, QSettings* settings);

  //QString market() const {return marketComboBox->currentText();}
  //QString userName() const {return userEdit->text();}
  //QString key() const {return keyEdit->text();}
  //QString secret() const {return secretEdit->text();}

private:
  QSettings* settings;
  QLineEdit* dataServerEdit;
  QLineEdit* botServerEdit;
  QLineEdit* botUserEdit;
  QLineEdit* botPasswordEdit;
  QPushButton* okButton;

  virtual void showEvent(QShowEvent* event);
  virtual void accept();
};
