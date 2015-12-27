#include "notizenmainwindow.h"
#include <QApplication>
#include <QTranslator>
#include <QFile>
#include "defaultvalues.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QSettings settings("settings.ini",QSettings::IniFormat);
    QString lang=settings.value("language",QString("English")).toString();
    QTranslator translator;
    if(QFile("./localization/"+lang+".qm").exists())
    {
        translator.load("./localization/"+lang+".qm");
        a.installTranslator(&translator);
    }
    DefaultValuesInitializer init;
    init.initialize();
    NotizenMainWindow w;
    w.show();
    return a.exec();
}
