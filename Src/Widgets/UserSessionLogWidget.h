
#pragma once

class UserSessionLogWidget : public QWidget
{
  Q_OBJECT

public:
  UserSessionLogWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager);

  void saveState(QSettings& settings);

private slots:
  void checkAutoScroll(const QModelIndex&, int, int);
  void autoScroll(int, int);

private:
  UserSessionLogModel logModel;
  QTreeView* logView;
  bool autoScrollEnabled;
};
