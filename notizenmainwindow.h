#ifndef NOTIZENMAINWINDOW_H
#define NOTIZENMAINWINDOW_H

#include <QMainWindow>
#include <QIcon>
#include "notesinternals.h"
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
    qint16 updateTags;
    QString filterString;

    std::vector<CategoryPair> categoryPairList;
    std::vector<EntryPair> entryPairList;

    void syncModelAndUI();
private slots:
    void categoryListChanged() {updateTags|=CategoryListChanged;}
    void categorySelectionChanged() {updateTags|=CategorySelectionChanged;}
    void categoryContentChanged() {updateTags|=EntryListContentChanged;}
    void entrySelectionChanged() {updateTags|=EntrySelectionChanged;}
    void entryContentChanged() {updateTags|=EntryContentChanged;}
    void on_savePushButton_clicked();
    void on_entriesListWidget_pressed(const QModelIndex &index);
    void on_removeEntryPushButton_clicked();
};

#endif // NOTIZENMAINWINDOW_H
