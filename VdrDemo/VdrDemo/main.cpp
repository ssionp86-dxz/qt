#include "mainwindow.h"
#include <QtWidgets/QApplication>
#include <QDir>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QDir::setCurrent(QCoreApplication::applicationDirPath());

    MainWindow w;
    w.show();
    return a.exec();
}