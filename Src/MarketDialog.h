
#pragma once

class MarketDialog : public QDialog
{
  Q_OBJECT

public:
  MarketDialog(QWidget* parent, Entity::Manager& entityManager);

  quint64 getMarketAdapterId() const;
  QString getUserName() const {return userEdit->text();}
  QString getKey() const {return keyEdit->text();}
  QString getSecret() const {return secretEdit->text();}

private:
  QLineEdit* userEdit;
  QLineEdit* keyEdit;
  QLineEdit* secretEdit;
  QComboBox* marketComboBox;
  QPushButton* okButton;

  virtual void showEvent(QShowEvent* event);

private slots:
  void textChanged();
};
