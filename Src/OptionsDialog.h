
#pragma once

class OptionsDialog : public QDialog
{
public:
  OptionsDialog(QWidget* parent, QSettings* settings);

private:
  QSettings* settings;
  QLineEdit* dataServerEdit;
  QLineEdit* dataUserEdit;
  QLineEdit* dataPasswordEdit;
  QPushButton* okButton;

  virtual void showEvent(QShowEvent* event);
  virtual void accept();
};
