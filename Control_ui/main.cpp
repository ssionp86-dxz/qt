#include "mainwindow.h"

#include "stylehelper.h"

#include <QApplication>
#include <QFont>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QFont font(QString::fromUtf8("Microsoft YaHei UI"));
    if (!font.exactMatch()) {
        font = QFont(QString::fromUtf8("Segoe UI"));
    }
    font.setPointSize(10);
    app.setFont(font);
    app.setStyle(QStringLiteral("Fusion"));

    app.setStyleSheet(StyleHelper::loadStyleSheet(QStringLiteral(":/styles/main.qss"))
                      + QStringLiteral("\n")
                      + StyleHelper::loadStyleSheet(QStringLiteral(":/styles/detailpages.qss")));

    MainWindow window;
    window.show();

    return app.exec();
}
