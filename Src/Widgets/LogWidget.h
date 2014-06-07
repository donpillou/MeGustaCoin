
#pragma once

class LogWidget : public QWidget
{
  Q_OBJECT

public:
  LogWidget(QWidget* parent, QSettings& settings, Entity::Manager& entityManager);

  void saveState(QSettings& settings);

private slots:
  void checkAutoScroll(const QModelIndex&, int, int);
  void autoScroll(int, int);

private:
  LogModel logModel;
  QTreeView* logView;
  bool autoScrollEnabled;
};
