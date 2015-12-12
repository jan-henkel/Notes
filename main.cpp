#include "notizenmainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    NotizenMainWindow w;
    w.show();

    return a.exec();
}
