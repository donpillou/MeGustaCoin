
#pragma once

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow();
  ~MainWindow();

private:
  QSettings settings;
  QTreeView* orderView;
  QSortFilterProxyModel* orderProxyModel;

  Market* market;

  void open(const QString& market, const QString& userName, const QString& key, const QString& secret);
  void updateStatusBar();

  virtual void closeEvent(QCloseEvent* event);

private slots:
  void login();
  void logout();
  void refresh();
};
