
#include "stdafx.h"

int main(int argc, char **argv)
{
  QApplication app (argc, argv);

#ifdef _WIN32
  app.setStyleSheet("QTabBar::close-button {background-image: url(\":/Icons/close_cold.png\");} QTabBar::close-button:hover {background-image: url(\":/Icons/close_hot.png\");}");
#endif

  MainWindow mainWindow;
  mainWindow.show();

  return app.exec();
}
