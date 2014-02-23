
#pragma once

class TradesWidget : public QWidget
{
  Q_OBJECT

public:
  TradesWidget(QWidget* parent, QSettings& settings, PublicDataModel& publicDataModel);

  void saveState(QSettings& settings);

public slots:
  void updateTitle();

private slots:
  void checkAutoScroll(const QModelIndex&, int, int);
  //void autoScroll(int, int);
  void clearAbove();

private:
  PublicDataModel& publicDataModel;
  TradeModel& tradeModel;
  QTreeView* tradeView;
  //bool autoScrollEnabled;
};
