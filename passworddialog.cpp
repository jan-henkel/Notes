#include "passworddialog.h"
#include "ui_passworddialog.h"

PasswordDialog::PasswordDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PasswordDialog)
{
    ui->setupUi(this);
}

void PasswordDialog::setMode(PasswordDialog::Mode mode)
{
    mode_=mode;
    setWindowTitle(windowTitleString_[mode_]);
    ui->label->setText(labelString_[mode_]);
    ui->labelConfirm->setVisible(confirmPW_[mode_]);
    ui->confirmLineEdit->setVisible(confirmPW_[mode_]);
    ui->generateMasterKeyCheckbox->setVisible(createMasterKeyOption_[mode_]);
    ui->passwordLineEdit->setEchoMode(QLineEdit::Password);
    ui->confirmLineEdit->setEchoMode(QLineEdit::Password);
    ui->passwordLineEdit->setInputMethodHints(Qt::ImhSensitiveData|ui->passwordLineEdit->inputMethodHints());
    ui->confirmLineEdit->setInputMethodHints(Qt::ImhSensitiveData|ui->confirmLineEdit->inputMethodHints());
    this->adjustSize();
}

int PasswordDialog::showWithMode(PasswordDialog::Mode mode)
{
    setMode(mode);
    return exec();
}

PasswordDialog::~PasswordDialog()
{
    delete ui;
}

void PasswordDialog::on_showPassword_clicked(bool checked)
{
    ui->passwordLineEdit->setEchoMode(!checked?QLineEdit::Password:QLineEdit::Normal);
    ui->confirmLineEdit->setEchoMode(ui->passwordLineEdit->echoMode());
}

void PasswordDialog::on_buttonBox_accepted()
{
    QDialog::accept();
    if(mode_==AskPassword)
    {
        emit passwordEntered(QCA::SecureArray(ui->passwordLineEdit->text().toUtf8()));
        ui->passwordLineEdit->setText("");
        return;
    }
    else
    {
        if(ui->passwordLineEdit->text()!=ui->confirmLineEdit->text())
        {
            emit passwordMismatch();
            return;
        }
        else
        {
            emit newPasswordSet(QCA::SecureArray(ui->passwordLineEdit->text().toUtf8()),createMasterKeyOption_[mode_]?ui->generateMasterKeyCheckbox->checkState():(mode_==CreateMasterKey));
            return;
        }
    }
}

void PasswordDialog::on_buttonBox_rejected()
{
    QDialog::reject();
}
