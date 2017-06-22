#ifndef NOTIZENTEXTEDIT_H
#define NOTIZENTEXTEDIT_H

#include <QObject>
#include <QWidget>
#include <QTextBrowser>
#include <QTextEdit>
#include <QMouseEvent>
#include <QTextCursor>
#include "cryptointerface.h"

class NotizenTextEdit : public QTextBrowser
{
    Q_OBJECT
public:
    NotizenTextEdit(QWidget *parent);
    CryptoPP::SecByteBlock toHtml() const;
    void insertHtml(const CryptoPP::SecByteBlock &text);
    void setHtml(const CryptoPP::SecByteBlock &text);
protected:
    void keyPressEvent(QKeyEvent *ev);
private:
    QTextCharFormat previousTextCharFormat;
    bool isHyperlink(const QTextCharFormat &format);
private slots:
    void textCharFormatChanged(QTextCharFormat newFormat);
};

#endif // NOTIZENTEXTEDIT_H
