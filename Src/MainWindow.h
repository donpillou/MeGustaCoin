
#pragma once

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow();
  ~MainWindow();

signals:
  void marketChanged(Market* market);

private:
  QSettings settings;

  OrderWidget* orderWidget;

  Market* market;
  QString marketName;
  QString userName;

  void open(const QString& market, const QString& userName, const QString& key, const QString& secret);

  virtual void closeEvent(QCloseEvent* event);

private slots:
  void login();
  void logout();
  void refresh();
  void updateWindowTitle();
  void about();
};
