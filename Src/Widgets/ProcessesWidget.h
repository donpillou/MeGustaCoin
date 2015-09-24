
#pragma once

class ProcessesWidget : public QWidget
{
public:
  ProcessesWidget(QWidget* parent, QSettings& settings, Entity::Manager& entityManager);

  void saveState(QSettings& settings);

private:
  ProcessModel processModel;
  QTreeView* processView;
};
