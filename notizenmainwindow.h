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
#include <QMenu>
#include <QStringList>
#include <functional>
#include "passworddialog.h"
#include <QShortcut>
#include "settingsdialog.h"
#include <QSettings>

namespace Ui {
class NotizenMainWindow;
}

class NotizenMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    //Severeal flags to signify the need to update certain UI elements
    enum UpdateFlags {
        CategoryListChanged=1<<0,
        CategorySelectionChanged=1<<1,
        EntryListContentChanged=1<<2,
        EntrySelectionChanged=1<<3,
        EntryContentChanged=1<<4
    };

    explicit NotizenMainWindow(QWidget *parent = 0);
    ~NotizenMainWindow();

    //high level functions to be called directly by user interactions.
    //they take care of the interaction with the underlying model, as well as requesting the necessary UI updates
    //they always refer to the currently selected/viewed category or entry

    //pertaining to current category/entry
    void addCategory();
    void addEntry();
    void removeCategory();
    void removeEntry();
    void printCategory();
    void printEntry();
    void saveEntry();
    void renameCategory();
    void renameEntry();
    void moveEntry(CategoryPair newCategory);
    void selectCategory();
    void selectEntry();
    //encryption
    void toggleEncryption();
    //settings
    void openSettings();
private:
    Ui::NotizenMainWindow *ui;
    //object to carry out all of the database operations, like adding and removing categories and entries.
    //also used to track currently selected category and entry
    NotesInternals notesInternals;
    //boolean to represent unsaved changes to the entry currently viewed
    bool edited_;
    //integer to store flags signifying necessary UI updates
    qint16 updateFlags;
    //string to store entry name filter, as entered into the entry search field (might not be necessary)
    QString filterString;

    //ctrl+s shortcut to save entry changes
    //QShortcut saveEntryShortcut;

    //variables to later store settings read from settings.ini
    QTextCharFormat defaultTextCharFormat;
    QFont entryFont;
    QColor entryFontColor;
    QFont printingFontCategory;
    QFont printingFontEntry;

    //settings dialog, non-modal
    SettingsDialog *settingsDialog;

    //vectors containing CategoryPair and EntryPair elements corresponding to the categories and entries
    //listed in UI-widgets. used to interact with notesInternals object
    std::vector<CategoryPair> categoryPairList;
    std::vector<EntryPair> entryPairList;

    //overload showEvent() for the purpose of loading settings once the window is visible (important, because frameGeometry() will return spurious values otherwise)
    void showEvent(QShowEvent *e);

    //function to synchronize the GUI-widgets with the underlying "model", i.e. notesInternals
    void syncModelAndUI();

    //function reading and applying everything from settings.ini
    void readSettings();

    //auxiliary functions for specific purposes

    //add or edit the URL associated with the currently selected text (creates clickable links)
    void editEntryTextURL();
    //sets edited_ boolean and changes UI accordingly (red floppy disk icon for unsaved changes)
    void setEdited(bool edited);
    bool edited() {return edited_;}
    //asks user about unsaved changes (if any), saves changes if requested
    void saveChanges();
    //toggles 'always on top' setting for the window (sometimes doesn't work, nothing to do with the code though)
    void toggleStayOnTop(bool stayOnTop);

    //event filter to handle user interactions that wouldn't otherwise be dealt with,
    //like selecting an entry with the arrow keys instead of the mouse, or deleting an entry with 'del'
    bool eventFilter(QObject *target, QEvent *e);

    //calls saveChanges to deal with unsaved changes
    void closeEvent(QCloseEvent *e);

    //set up input dialog without "what's this?" button and expanded to fit the content
    void setUpModalInputDialog(QInputDialog &inputDialog, QString windowTitle, QString labelText, QInputDialog::InputMode inputMode=QInputDialog::TextInput, QString textValue=QString(""));
private slots:
    //slots to be called by notesInternals when changes pertaining to selected (and therefore visible) categories or entries occur.
    //they set the respective UI-update flags
    void categoryListChanged() {updateFlags|=CategoryListChanged;}
    void categorySelectionChanged() {updateFlags|=CategorySelectionChanged;}
    void categoryContentChanged() {updateFlags|=EntryListContentChanged;}
    void entrySelectionChanged() {updateFlags|=EntrySelectionChanged;}
    void entryContentChanged() {updateFlags|=EntryContentChanged;}

    //slots to be called by password dialog

    //sets a new password. also creates a new master key if desired (in which case all encrypted entries are written to files again, using the new master key)
    void createNewPassword(QCA::SecureArray password, bool createMasterKey);
    //attempts to use entered password to decrypt
    void passwordEntered(QCA::SecureArray password);
    //notifies the user that the contents of the 2 password fields don't match (when setting a new password)
    void passwordMismatch();

    //apply changes made in the settings dialog
    void settingsDialogApply();
    //password change requested by settings dialog
    void settingsChangePassword();

    //called by the "Move entry" context menu. moves current entry to the category selected in the menu
    //using high level function moveEntry
    void moveEntryMenu(QAction *action);
    //action slots for "rename" menu items for categories and entries. taken care of by high level functions
    void on_actionRenameCategory_triggered();
    void on_actionRenameEntry_triggered();
    //action slots for "delete" menu items for categories and entries. again taken care of by high level functions
    void on_actionDeleteEntry_triggered();
    void on_actionDeleteCategory_triggered();

    //slots for various UI elements. mostly just call high level functions

    //category buttons
    void on_addCategoryPushButton_clicked();
    void on_removeCategoryPushButton_clicked();
    void on_printCategoryPushButton_clicked();
    //entry buttons
    void on_addEntryPushButton_clicked();
    void on_removeEntryPushButton_clicked();
    void on_printEntryPushButton_clicked();
    //category combo box
    void on_categoriesComboBox_activated(int index);
    void on_categoriesComboBox_customContextMenuRequested(const QPoint &pos);
    //entry search field
    void on_entryFilterLineEdit_textEdited(const QString &arg1);
    void on_entryFilterLineEdit_returnPressed();
    //buttons in the top right
    void on_encryptionPushButton_clicked();
    void on_stayOnTopToolButton_clicked(bool checked);
    void on_savePushButton_clicked();
    void on_settingsPushButton_clicked();
    //entry list
    void on_entriesListWidget_customContextMenuRequested(const QPoint &pos);
    void on_entriesListWidget_pressed(const QModelIndex &index);
    //entry text editor
    void on_entryTextEdit_anchorClicked(const QUrl &arg1);
    void on_entryTextEdit_currentCharFormatChanged(const QTextCharFormat &format);
    void on_entryTextEdit_customContextMenuRequested(const QPoint &pos);
    void on_entryTextEdit_textChanged();
    //text formatting widgets
    void on_fontComboBox_activated(const QString &arg1);
    void on_makeLinkCheckBox_clicked(bool checked);
    void on_colorPushButton_clicked();
    void on_fontSizeSpinBox_editingFinished();
    void on_italicToolButton_clicked(bool checked);
    void on_underlineToolButton_clicked(bool checked);
    void on_boldToolButton_clicked(bool checked);

    //slot to handle URL clicked in entry text widget
    void on_actionEditURL_triggered();

    //slot to be called by save entry shortcut
    //void saveEntryShortcutTriggered();
};

#endif // NOTIZENMAINWINDOW_H
