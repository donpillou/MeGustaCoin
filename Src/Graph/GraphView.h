
#pragma once

class GraphView : public QWidget
{
public:
  GraphView(QWidget* parent, GraphModel& graphModel);

  //void setMaxAge(int maxAge);
  //int getMaxAge() const {return maxAge;}
  //
  //void setEnabledData(unsigned int data);
  //unsigned int getEnabledData() const {return enabledData;}

  virtual QSize sizeHint() const;

private:
  //Entity::Manager& globalEntityManager;
  //Entity::Manager& channelEntityManager;
  GraphModel& graphModel;
  //EDataSubscription* eDataSubscription;

  //unsigned int enabledData;
  //int maxAge;

private: // QWidget
  virtual void resizeEvent(QResizeEvent* event);
  virtual void paintEvent(QPaintEvent* event);
};
