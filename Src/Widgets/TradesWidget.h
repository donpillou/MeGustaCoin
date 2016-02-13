
#pragma once

class TradesWidget : public QWidget, public Entity::Listener
{
  Q_OBJECT

public:
  TradesWidget(QTabFramework& tabFramework, QSettings& settings, const QString& channelName, Entity::Manager& channelEntityManager);
  ~TradesWidget();

  void saveState(QSettings& settings);

  void updateTitle();

private:
  QTabFramework& tabFramework;
  QString channelName;
  Entity::Manager& channelEntityManager;
  MarketTradesModel tradesModel;
  QTreeView* tradeView;

private slots:
  void rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end);

private: // Entity::Listener
  virtual void updatedEntitiy(Entity& oldEntity, Entity& newEntity);
};
