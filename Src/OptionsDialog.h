
#pragma once

class OptionsDialog : public QDialog
{
public:
  OptionsDialog(QWidget* parent, QSettings* settings);

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
