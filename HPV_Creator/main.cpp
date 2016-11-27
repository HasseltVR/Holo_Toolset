#include "mainwindow.hpp"
#include <QApplication>
#include <QtWidgets>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(app.translate("main", "HPV Creator"));
#ifdef Q_WS_MAC
    app.setCursorFlashTime(0);
#endif

    //QIcon icon("icon.ico");

    MainWindow mainWindow;
    mainWindow.setMinimumWidth(800);
    mainWindow.show();
    return app.exec();
}
