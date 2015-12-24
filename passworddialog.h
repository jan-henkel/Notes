#ifndef NEWPASSWORDDIALOG_H
#define NEWPASSWORDDIALOG_H

#include <QDialog>
#include <QtCrypto>
#include <QKeyEvent>
#include <QEvent>
namespace Ui {
class PasswordDialog;
}

class PasswordDialog : public QDialog
{
    Q_OBJECT

public:
    enum Mode {ChangePassword,CreateMasterKey,AskPassword};
    explicit PasswordDialog(QWidget *parent = 0);
    void setMode(Mode mode);
    int showWithMode(Mode mode);
    ~PasswordDialog();
signals:
    void newPasswordSet(QCA::SecureArray password,bool generateMasterKey);
    void passwordEntered(QCA::SecureArray password);
    void passwordMismatch();
private slots:
    void on_showPassword_clicked(bool checked);
    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

private:
    Mode mode_;
    QString windowTitleString_[3]={"Change password","Create master key","Enter password"};
    QString labelString_[3]={"Enter your new password twice and press Ok to confirm.", "Pick a password, enter it twice and press Ok to confirm.", "Enter your current password."};
    bool confirmPW_[3]={true,true,false};
    bool createMasterKeyOption_[3]={true,false,false};
    Ui::PasswordDialog *ui;
    void proceed();
    void keyPressEvent(QKeyEvent *e);
};

#endif // NEWPASSWORDDIALOG_H
