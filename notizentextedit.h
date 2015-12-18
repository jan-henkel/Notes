#ifndef NOTIZENTEXTEDIT_H
#define NOTIZENTEXTEDIT_H

#include <QObject>
#include <QWidget>
#include <QTextBrowser>
#include <QTextEdit>
#include <QMouseEvent>
#include <QTextCursor>

class NotizenTextEdit : public QTextBrowser
{
    Q_OBJECT
public:
    NotizenTextEdit(QWidget *parent);
protected:
    void mouseMoveEvent(QMouseEvent * event);
    void mousePressEvent(QMouseEvent * event);
};

#endif // NOTIZENTEXTEDIT_H
