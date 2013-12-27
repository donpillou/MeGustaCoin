
#pragma once

class TradesWidget : public QWidget
{
  Q_OBJECT

public:
  TradesWidget(QWidget* parent, QSettings& settings, DataModel& dataModel);

  void saveState(QSettings& settings);

private slots:
  void setMarket(Market* market);
  void checkAutoScroll(const QModelIndex&, int, int);
  void autoScroll(int, int);
  void clearAbove();

private:
  DataModel& dataModel;
  TradeModel& tradeModel;
  Market* market;
  QTreeView* tradeView;
  bool autoScrollEnabled;
};
