
#pragma once

class BotOrdersWidget : public QWidget
{
public:
  BotOrdersWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager);

  void saveState(QSettings& settings);

private:
  UserSessionOrdersModel ordersModel;
  QTreeView* orderView;
};
