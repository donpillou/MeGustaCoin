
#pragma once

class BotOrdersWidget : public QWidget
{
public:
  BotOrdersWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager);

  void saveState(QSettings& settings);

private:
  QTabFramework& tabFramework;
  Entity::Manager& entityManager;

  SessionOrderModel orderModel;

  QTreeView* orderView;
};
