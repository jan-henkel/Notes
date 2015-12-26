#include "notizenmainwindow.h"
#include <QApplication>
#include <QTranslator>
#include "defaultvalues.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    DefaultValuesInitializer init;
    init.initialize();
    NotizenMainWindow w;
    w.setWindowFlags(w.windowFlags()|Qt::CustomizeWindowHint|Qt::WindowStaysOnTopHint);
    w.show();
    /*QTranslator translator;
    translator.load("ger");
    a.installTranslator(&translator);*/
    return a.exec();
}
