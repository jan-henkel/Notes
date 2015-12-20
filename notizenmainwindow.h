#ifndef NOTIZENMAINWINDOW_H
#define NOTIZENMAINWINDOW_H

#include <QMainWindow>
#include <QIcon>
#include <QtPrintSupport/QtPrintSupport>
#include <QtPrintSupport/QPrintDialog>
#include <QtPrintSupport/QPrinter>
#include <QtPrintSupport/QPrintEngine>
#include <QTextDocument>
#include "notesinternals.h"
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QTextCharFormat>
#include <QTextBlockFormat>
#include <QFontDialog>
#include <QInputDialog>

namespace Ui {
class NotizenMainWindow;
}

class NotizenMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    enum UpdateTags {
        CategoryListChanged=1<<0,
        CategorySelectionChanged=1<<1,
        EntryListContentChanged=1<<2,
        EntrySelectionChanged=1<<3,
        EntryContentChanged=1<<4
    };

    explicit NotizenMainWindow(QWidget *parent = 0);
    ~NotizenMainWindow();
    void addCategory();
    void addEntry();
    void saveEntry();
    void removeEntry();
    void removeCategory();
    void printEntry();
    void printCategory();
    void toggleEncryption();
private slots:
    void on_addCategoryPushButton_clicked();

    void on_addEntryPushButton_clicked();

    void on_categoriesComboBox_activated(int index);

    void on_entriesListWidget_currentRowChanged(int currentRow);

    void on_entryFilterLineEdit_textEdited(const QString &arg1);
private:
    Ui::NotizenMainWindow *ui;
    NotesInternals notesInternals;
    bool edited;
    qint16 updateFlags;
    QString filterString;
    QTextBlockFormat defaultTextBlockFormat;
    QTextCharFormat defaultTextCharFormat;

    std::vector<CategoryPair> categoryPairList;
    std::vector<EntryPair> entryPairList;

    void syncModelAndUI();
private slots:
    void categoryListChanged() {updateFlags|=CategoryListChanged;}
    void categorySelectionChanged() {updateFlags|=CategorySelectionChanged;}
    void categoryContentChanged() {updateFlags|=EntryListContentChanged;}
    void entrySelectionChanged() {updateFlags|=EntrySelectionChanged;}
    void entryContentChanged() {updateFlags|=EntryContentChanged;}
    void on_savePushButton_clicked();
    void on_entriesListWidget_pressed(const QModelIndex &index);
    void on_removeEntryPushButton_clicked();
    void on_printEntryPushButton_clicked();
    void on_removeCategoryPushButton_clicked();
    void on_entryTextEdit_anchorClicked(const QUrl &arg1);
    void on_entryTextEdit_cursorPositionChanged();
    void on_entryTextEdit_textChanged();
    void on_printCategoryPushButton_clicked();
    void on_fontComboBox_activated(const QString &arg1);
    void on_makeLinkCheckBox_clicked(bool checked);
    void on_colorPushButton_clicked();
    void on_fontSizeSpinBox_valueChanged(int arg1);
    void on_fontSizeSpinBox_editingFinished();
    void on_italicToolButton_clicked(bool checked);
    void on_underlineToolButton_clicked(bool checked);
    void on_boldToolButton_clicked(bool checked);
    void on_entriesListWidget_customContextMenuRequested(const QPoint &pos);
    void on_encryptionPushButton_clicked();
};

#endif // NOTIZENMAINWINDOW_H
