
#pragma once

class BotLogWidget : public QWidget
{
  Q_OBJECT

public:
  BotLogWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager);

  void saveState(QSettings& settings);

private slots:
  void checkAutoScroll(const QModelIndex&, int, int);
  void autoScroll(int, int);

private:
  SessionLogModel logModel;
  QTreeView* logView;
  bool autoScrollEnabled;
};
