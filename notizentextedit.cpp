#include "notizentextedit.h"

NotizenTextEdit::NotizenTextEdit(QWidget *parent):QTextBrowser(parent)
{
    QObject::connect((QTextEdit*)this,SIGNAL(currentCharFormatChanged(QTextCharFormat)),this,SLOT(textCharFormatChanged(QTextCharFormat)));
}

QCA::SecureArray NotizenTextEdit::toHtml() const
{
    return QCA::SecureArray(((QTextBrowser*)this)->toHtml().toUtf8());
}

void NotizenTextEdit::insertHtml(const QCA::SecureArray &text)
{
    ((QTextBrowser*)this)->insertHtml(QString(QByteArray(text.constData(),text.size())));
}

void NotizenTextEdit::setHtml(const QCA::SecureArray &text)
{
    ((QTextBrowser*)this)->setHtml(QString(QByteArray(text.constData(),text.size())));
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
