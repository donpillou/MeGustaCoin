
#pragma once

class BotLogWidget : public QWidget, public Entity::Listener
{
  Q_OBJECT

public:
  BotLogWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager);
  ~BotLogWidget();

  void saveState(QSettings& settings);

private slots:
  void checkAutoScroll(const QModelIndex&, int, int);
  void autoScroll(int, int);

private:
  QTabFramework& tabFramework;
  Entity::Manager& entityManager;
  SessionLogModel logModel;
  QTreeView* logView;
  bool autoScrollEnabled;

private:
  void updateTitle(EBotService& eBotService);

private: // Entity::Listener
  virtual void updatedEntitiy(Entity& oldEntity, Entity& newEntity);
};
