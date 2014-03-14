
#pragma once

class BotService : public QObject
{
  Q_OBJECT

public:
  BotService();
  ~BotService();

  void start();
  void stop();

private:
  class Action
  {
  public:
    virtual void execute(BotService& botService) = 0;
  };

  QThread* thread;


  JobQueue<Action*> actionQueue;
  //JobQueue<Action*> subscriptionQueue;
  //QHash<quint64, PublicDataModel*> activeSubscriptions;
  //bool isConnected;
  //QSet<QString> subscriptions;

private slots:
  void executeActions();
};
