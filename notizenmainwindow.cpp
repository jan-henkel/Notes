#include "notizenmainwindow.h"
#include "ui_notizenmainwindow.h"
#include <vector>

NotizenMainWindow::NotizenMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::NotizenMainWindow),
    notesInternals(this)
{
    ui->setupUi(this);
}

NotizenMainWindow::~NotizenMainWindow()
{
    delete ui;
}
