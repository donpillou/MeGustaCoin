
#pragma once

class TradesWidget : public QWidget, public Entity::Listener
{
public:
  TradesWidget(QWidget* parent, QSettings& settings, const QString& channelName, Entity::Manager& channelEntityManager);
  ~TradesWidget();

  void saveState(QSettings& settings);

  void updateTitle();

private:
  QString channelName;
  Entity::Manager& channelEntityManager;
  TradeModel tradeModel;
  QTreeView* tradeView;

private: // Entity::Listener
  virtual void updatedEntitiy(Entity& oldEntity, Entity& newEntity);
};
