
#pragma once

class BotItemsWidget : public QWidget
{
public:
  BotItemsWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager);

  void saveState(QSettings& settings);

private:
  QTabFramework& tabFramework;
  Entity::Manager& entityManager;

  SessionItemModel itemModel;

  QTreeView* itemView;
};
