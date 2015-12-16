#include "notizenmainwindow.h"
#include "ui_notizenmainwindow.h"
#include <vector>

NotizenMainWindow::NotizenMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::NotizenMainWindow),
    notesInternals(this)
{
    ui->setupUi(this);
    updateListsAndUI();
}

NotizenMainWindow::~NotizenMainWindow()
{
    delete ui;
}

void NotizenMainWindow::addCategory(QString categoryName)
{
    selectCategoryIterator(notesInternals.addCategory(categoryName),true);
    updateListsAndUI();
}

void NotizenMainWindow::addEntry(QString entryName)
{
    if(currentCategory)
        selectEntryIterator(notesInternals.addEntry(currentCategoryIterator,entryName));
    updateEntryListAndUI();
    updateEntryTextUI();
}

void NotizenMainWindow::saveEntry()
{
    if(currentCategory && currentEntry)
    {
        notesInternals.modifyEntryText(currentCategoryIterator,currentEntryIterator,ui->entryTextEdit->toPlainText());
    }
}

void NotizenMainWindow::selectCategoryIterator(CategoriesMap::const_iterator newCategoryIterator, bool resetEntry)
{
    currentCategoryIterator=newCategoryIterator;
    currentCategory=NotesInternals::getCategory(currentCategoryIterator);
    if(resetEntry)
    {
        currentEntryIterator=currentCategory->entriesMap()->end();
        currentEntry=0;
    }
}

void NotizenMainWindow::selectEntryIterator(EntriesMap::const_iterator newEntryIterator)
{
    currentEntryIterator=newEntryIterator;
    currentEntry=NotesInternals::getEntry(currentEntryIterator);
}

void NotizenMainWindow::updateCategoryListAndUI()
{
    categoryIteratorList.clear();
    for(CategoriesMap::const_iterator i=notesInternals.categoriesMap()->begin();i!=notesInternals.categoriesMap()->end();++i)
        categoryIteratorList.push_back(i);
    ui->categoriesComboBox->clear();
    int j=-1;
    for(unsigned int i=0;i<categoryIteratorList.size();++i)
    {
        ui->categoriesComboBox->addItem(NotesInternals::getCategoryEncrypted(categoryIteratorList[i])?QIcon("/icons/lock_add.png"):QIcon(),
                                        NotesInternals::getCategoryName(categoryIteratorList[i]));
        if(currentCategory==NotesInternals::getCategory(categoryIteratorList[i]))
            j=i;
    }
    if(j==-1)
    {
        currentCategory=0;
    }
    ui->categoriesComboBox->setCurrentIndex(j);
}

void NotizenMainWindow::updateEntryListAndUI()
{
    entryIteratorList.clear();
    ui->entriesListWidget->clear();
    if(currentCategory)
    {
        int j=-1;
        for(EntriesMap::const_iterator i=currentCategory->entriesMap()->cbegin();i!=currentCategory->entriesMap()->cend();++i)
        {
            if(NotesInternals::getEntryName(i).startsWith(ui->entryFilterLineEdit->text(),Qt::CaseInsensitive))
                entryIteratorList.push_back(i);
        }
        for(EntriesMap::const_iterator i=currentCategory->entriesMap()->cbegin();i!=currentCategory->entriesMap()->cend();++i)
        {
            if(!NotesInternals::getEntryName(i).startsWith(ui->entryFilterLineEdit->text(),Qt::CaseInsensitive) && NotesInternals::getEntryName(i).contains(ui->entryFilterLineEdit->text(),Qt::CaseInsensitive))
                entryIteratorList.push_back(i);
        }
        for(unsigned int i=0;i<entryIteratorList.size();++i)
        {
            ui->entriesListWidget->addItem(NotesInternals::getEntryName(entryIteratorList[i]));
            if(currentEntry==NotesInternals::getEntry(entryIteratorList[i]))
                j=i;
        }
        if(j==-1)
        {
            currentEntryIterator=currentCategory->entriesMap()->cend();
            currentEntry=0;
            updateEntryTextUI();
        }
        ui->entriesListWidget->setCurrentRow(j);
    }
}

void NotizenMainWindow::updateEntryTextUI()
{
    ui->entryTextEdit->clear();
    if(currentCategory && currentEntry)
    {
        ui->entryTextEdit->setText(NotesInternals::getEntryText(currentEntryIterator));
    }
}

void NotizenMainWindow::updateListsAndUI()
{
    updateCategoryListAndUI();
    updateEntryListAndUI();
    updateEntryTextUI();
}

void NotizenMainWindow::on_addCategoryPushButton_clicked()
{
    addCategory(ui->categoriesComboBox->currentText());
}

void NotizenMainWindow::on_addEntryPushButton_clicked()
{
    addEntry(ui->entryFilterLineEdit->text());
}

void NotizenMainWindow::on_categoriesComboBox_activated(int index)
{
    selectCategoryIterator(categoryIteratorList[index],true);
    updateListsAndUI();
}

void NotizenMainWindow::on_entriesListWidget_activated(const QModelIndex &index)
{

}

void NotizenMainWindow::on_printPushButton_clicked()
{
    saveEntry();
}


void NotizenMainWindow::on_entriesListWidget_entered(const QModelIndex &index)
{

}

void NotizenMainWindow::on_entriesListWidget_currentRowChanged(int currentRow)
{

}

void NotizenMainWindow::on_entryFilterLineEdit_textChanged(const QString &arg1)
{

}

void NotizenMainWindow::on_entryFilterLineEdit_textEdited(const QString &arg1)
{
    updateEntryListAndUI();
}

void NotizenMainWindow::on_entriesListWidget_clicked(const QModelIndex &index)
{

}

void NotizenMainWindow::on_entriesListWidget_itemSelectionChanged()
{

}

void NotizenMainWindow::on_entriesListWidget_pressed(const QModelIndex &index)
{
    int i=ui->entriesListWidget->currentIndex().row();
    if(i>=0 && i<entryIteratorList.size())
    {
        selectEntryIterator(entryIteratorList[i]);
        updateEntryTextUI();
    }
}

void NotizenMainWindow::on_encryptionPushButton_clicked()
{

}
