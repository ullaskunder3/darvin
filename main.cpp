#include "mainwindow.h"
#include <QApplication>
#include <QDockWidget>
#include <QTreeView>
#include <QFileSystemModel>
#include <QDir>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return app.exec();
}
