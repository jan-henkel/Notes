#include "notizenmainwindow.h"
#include <QApplication>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    NotizenMainWindow w;
    w.setWindowFlags(w.windowFlags()|Qt::CustomizeWindowHint|Qt::WindowStaysOnTopHint);
    w.show();

    /*QTranslator translator;
    translator.load("ger");
    a.installTranslator(&translator);*/
    return a.exec();
}
