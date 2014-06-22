
#pragma once

class TradesWidget : public QWidget, public Entity::Listener
{
public:
  TradesWidget(QTabFramework& tabFramework, QSettings& settings, const QString& channelName, Entity::Manager& channelEntityManager);
  ~TradesWidget();

  void saveState(QSettings& settings);

  void updateTitle();

private:
  QTabFramework& tabFramework;
  QString channelName;
  Entity::Manager& channelEntityManager;
  TradeModel tradeModel;
  QTreeView* tradeView;

private: // Entity::Listener
  virtual void updatedEntitiy(Entity& oldEntity, Entity& newEntity);
};
