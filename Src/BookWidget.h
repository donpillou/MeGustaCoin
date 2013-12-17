
#pragma once

class BookWidget : public QWidget
{
  Q_OBJECT

public:
  BookWidget(QWidget* parent, QSettings& settings, DataModel& dataModel);

  void saveState(QSettings& settings);

private slots:
  void setMarket(Market* market);
  void checkAutoScroll(const QModelIndex&, int, int);
  void autoScroll(int, int);

private:
  DataModel& dataModel;
  BookModel& bookModel;
  Market* market;
  QTreeView* askView;
  QTreeView* bidView;
  bool askAutoScrollEnabled;
  bool bidAutoScrollEnabled;
};
