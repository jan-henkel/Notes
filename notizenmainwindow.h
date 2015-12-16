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

    CategoriesMap::const_iterator currentCategoryIterator;
    EntriesMap::const_iterator currentEntryIterator;
    const Category* currentCategory;
    const Entry* currentEntry;

    std::vector<CategoriesMap::const_iterator> categoryIteratorList;
    std::vector<EntriesMap::const_iterator> entryIteratorList;

    void selectCategoryIterator(CategoriesMap::const_iterator newCategoryIterator, bool resetEntry);
    void selectEntryIterator(EntriesMap::const_iterator newEntryIterator);

    void updateCategoryListAndUI();
    void updateEntryListAndUI();
    void updateEntryTextUI();
    void updateListsAndUI();
};

#endif // NOTIZENMAINWINDOW_H
