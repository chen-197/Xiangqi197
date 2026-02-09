#include "mainwindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[])
{
    // Better HiDPI behavior (Qt6): keep scale-factor rounding smooth and use HiDPI pixmaps.
    // These must be set before QApplication is constructed.
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication a(argc, argv);

    srand(time(0) + 4096);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "Xiangqi_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    MainWindow w;
    w.show();
    return a.exec();
}
