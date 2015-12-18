#include "notizentextedit.h"

NotizenTextEdit::NotizenTextEdit(QWidget *parent):QTextBrowser(parent)
{
this->setMouseTracking(true);
}

void NotizenTextEdit::mouseMoveEvent(QMouseEvent *event)
{
    QTextCursor cursor=cursorForPosition(event->pos());
    cursor.select(QTextCursor::WordUnderCursor);
    if(!QUrl(cursor.selectedText()).isValid())
        this->viewport()->setCursor(Qt::PointingHandCursor);
    else
        this->viewport()->setCursor(Qt::IBeamCursor);


}

void NotizenTextEdit::mousePressEvent(QMouseEvent *event)
{
    QTextCursor cursor=cursorForPosition(event->pos());
    cursor.select(QTextCursor::BlockUnderCursor);
    this->setHtml(cursor.selectedText());
    QTextEdit::mousePressEvent(event);
}

