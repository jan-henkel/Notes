#include "notizentextedit.h"

NotizenTextEdit::NotizenTextEdit(QWidget *parent):QTextBrowser(parent)
{
    QObject::connect((QTextEdit*)this,SIGNAL(currentCharFormatChanged(QTextCharFormat)),this,SLOT(textCharFormatChanged(QTextCharFormat)));
}

CryptoPP::SecByteBlock NotizenTextEdit::toHtml() const
{
    return CryptoInterface::toSecBlock(((QTextBrowser*)this)->toHtml().toUtf8());
}

void NotizenTextEdit::insertHtml(const CryptoPP::SecByteBlock &text)
{
    ((QTextBrowser*)this)->insertHtml(QString(CryptoInterface::toPermByteArray(text)));
}

void NotizenTextEdit::setHtml(const CryptoPP::SecByteBlock &text)
{
    ((QTextBrowser*)this)->setHtml(QString(CryptoInterface::toPermByteArray(text)));
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
