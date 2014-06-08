
#pragma once

class TradesWidget : public QWidget
{
  Q_OBJECT

public:
  TradesWidget(QWidget* parent, QSettings& settings, const QString& channelName, Entity::Manager& entityManager);

  void saveState(QSettings& settings);

public slots:
  void updateTitle(); // todo ???

private:
  QString channelName;
  Entity::Manager& entityManager;
  TradeModel tradeModel;
  QTreeView* tradeView;
};
