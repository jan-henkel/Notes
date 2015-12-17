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
        EntryListChanged=1<<1,
        EntryTextChanged=1<<2
    };

    explicit NotizenMainWindow(QWidget *parent = 0);
    ~NotizenMainWindow();
    void addCategory(QString categoryName);
    void addEntry(QString entryName);
    void saveEntry();
    void removeEntry();
private slots:
    void on_addCategoryPushButton_clicked();

    void on_addEntryPushButton_clicked();

    void on_categoriesComboBox_activated(int index);

    void on_entriesListWidget_activated(const QModelIndex &index);

    void on_printPushButton_clicked();
    void on_entriesListWidget_entered(const QModelIndex &index);

    void on_entriesListWidget_currentRowChanged(int currentRow);

    void on_entryFilterLineEdit_textChanged(const QString &arg1);

    void on_entryFilterLineEdit_textEdited(const QString &arg1);

    void on_entriesListWidget_clicked(const QModelIndex &index);

    void on_entriesListWidget_itemSelectionChanged();

    void on_entriesListWidget_pressed(const QModelIndex &index);

    void on_encryptionPushButton_clicked();
private:
    Ui::NotizenMainWindow *ui;
    NotesInternals notesInternals;
    bool edited;
    qint16 updateTags;
    QString filterString;

    CategoryPair currentCategoryPair;
    EntryPair currentEntryPair;

    std::vector<CategoryPair> categoryPairList;
    std::vector<EntryPair> entryPairList;

    void selectCategoryPair(CategoryPair newCategoryPair, bool resetEntry);
    void selectEntryPair(EntryPair newEntryPair);

    void updateCategoryListAndUI();
    void updateEntryListAndUI();
    void updateEntryTextUI();
    void updateListsAndUI();
    void syncModelAndUI();
private slots:
    void categoryListChanged() {updateTags|=CategoryListChanged;}
    void categoryChanged() {updateTags|=EntryListChanged;}
    void entryChanged() {updateTags|=EntryTextChanged;}
};

#endif // NOTIZENMAINWINDOW_H
