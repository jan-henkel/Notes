#include "notizenmainwindow.h"
#include <QApplication>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    NotizenMainWindow w;
    w.show();

    /*QTranslator translator;
    translator.load("ger");
    a.installTranslator(&translator);*/
    return a.exec();
}
