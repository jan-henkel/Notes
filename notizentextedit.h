#ifndef NOTIZENTEXTEDIT_H
#define NOTIZENTEXTEDIT_H

#include <QObject>
#include <QWidget>
#include <QTextBrowser>
#include <QTextEdit>
#include <QMouseEvent>
#include <QTextCursor>
#include <QtCrypto>
class NotizenTextEdit : public QTextBrowser
{
    Q_OBJECT
public:
    NotizenTextEdit(QWidget *parent);
    QCA::SecureArray toHtml() const;
    void insertHtml(const QCA::SecureArray &text);
    void setHtml(const QCA::SecureArray &text);
protected:
    void keyPressEvent(QKeyEvent *ev);
private:
    QTextCharFormat previousTextCharFormat;
    bool isHyperlink(const QTextCharFormat &format);
private slots:
    void textCharFormatChanged(QTextCharFormat newFormat);
};

#endif // NOTIZENTEXTEDIT_H
