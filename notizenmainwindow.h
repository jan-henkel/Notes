#ifndef NOTIZENMAINWINDOW_H
#define NOTIZENMAINWINDOW_H

#include <QMainWindow>
#include "notesinternals.h"
namespace Ui {
class NotizenMainWindow;
}

class NotizenMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit NotizenMainWindow(QWidget *parent = 0);
    ~NotizenMainWindow();

private:
    Ui::NotizenMainWindow *ui;
    NotesInternals notesInternals;
};

#endif // NOTIZENMAINWINDOW_H
