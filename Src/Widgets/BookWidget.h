
#pragma once

#if 0

class BookWidget : public QWidget
{
  Q_OBJECT

public:
  BookWidget(QWidget* parent, QSettings& settings, PublicDataModel& publicDataModel);

  void saveState(QSettings& settings);

public slots:
  void updateTitle();

private slots:
  void checkAutoScroll(const QModelIndex&, int, int);
  void autoScroll(int, int);

private:
  PublicDataModel& publicDataModel;
  BookModel& bookModel;
  QTreeView* askView;
  QTreeView* bidView;
  bool askAutoScrollEnabled;
  bool bidAutoScrollEnabled;
};
#endif