#include "notizentextedit.h"

NotizenTextEdit::NotizenTextEdit(QWidget *parent):QTextBrowser(parent)
{
    QObject::connect((QTextEdit*)this,SIGNAL(currentCharFormatChanged(QTextCharFormat)),this,SLOT(textCharFormatChanged(QTextCharFormat)));
}

void NotizenTextEdit::keyPressEvent(QKeyEvent *ev)
{
    if(isHyperlink(this->currentCharFormat()))
    {
        if(ev->text()==" " || ev->text()=="\r" || ev->text()=="\n")
            this->setCurrentCharFormat(previousTextCharFormat);
    }
    QTextBrowser::keyPressEvent(ev);
}

bool NotizenTextEdit::isHyperlink(const QTextCharFormat &format)
{
    return (format.anchorHref()!="" || format.anchorName()!="" || format.isAnchor()==true);
}

void NotizenTextEdit::textCharFormatChanged(QTextCharFormat newFormat)
{
    //save non hyperlink formatting
    if(!isHyperlink(newFormat))
        previousTextCharFormat=newFormat;
}
