#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QSettings>
#include <QFont>
#include <QTextCharFormat>
#include <QColor>
#include <QFontDialog>
#include <QTextBrowser>
#include <QTextCursor>
#include "notizentextedit.h"
#include "notesinternals.h"
#include "defaultvalues.h"

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();
    void showSettings(NotesInternals* notesInternals);
private slots:
    void on_applyPushButton_clicked();

private:
    NotesInternals *notesInternals_;
    Ui::SettingsDialog *ui;

    //a few settings can't easily be obtained from widget properties and thus are stored separately
    QColor entryFontColor_;
};

#endif // SETTINGSDIALOG_H
