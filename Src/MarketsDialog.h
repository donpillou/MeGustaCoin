
#pragma once

class MarketsDialog : public QDialog
{
  //Q_OBJECT

public:
  MarketsDialog(QWidget* parent, QSettings& settings, Entity::Manager& entityManager);

private:
  QSettings& settings;
  QTreeView* marketView;
  QSortFilterProxyModel* proxyModel;
  QPushButton* okButton;

  BotMarketModel botMarketModel;

  virtual void showEvent(QShowEvent* event);
  virtual void accept();

private slots:
  //void textChanged();
};
