
#pragma once

class UserSessionOrdersWidget : public QWidget
{
public:
  UserSessionOrdersWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager);

  void saveState(QSettings& settings);

private:
  UserSessionOrdersModel ordersModel;
  QTreeView* orderView;
};
