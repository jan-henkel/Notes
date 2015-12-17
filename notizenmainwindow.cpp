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
    selectCategoryPair(notesInternals.addCategory(categoryName),true);
    updateListsAndUI();
}

void NotizenMainWindow::addEntry(QString entryName)
{
    if(currentCategory)
        selectEntryPair(notesInternals.addEntry(currentCategoryPair,entryName));
    updateEntryListAndUI();
    updateEntryTextUI();
}

void NotizenMainWindow::saveEntry()
{
    if(currentCategory && currentEntry)
    {
        notesInternals.modifyEntryText(currentCategoryPair,currentEntryPair,ui->entryTextEdit->toPlainText());
    }
}

void NotizenMainWindow::selectCategoryPair(CategoryPair newCategoryPair, bool resetEntry)
{
    currentCategoryPair=newCategoryPair;
    currentCategory=NotesInternals::getCategory(currentCategoryPair);
    if(resetEntry)
    {
        currentEntryPair=EntryPair(NameDate(QString(""),QDateTime::fromMSecsSinceEpoch(0)),0);
        currentEntry=0;
    }
}

void NotizenMainWindow::selectEntryPair(EntryPair newEntryPair)
{
    currentEntryPair=newEntryPair;
    currentEntry=NotesInternals::getEntry(currentEntryPair);
}

void NotizenMainWindow::updateCategoryListAndUI()
{
    categoryPairList.clear();
    for(CategoriesMap::const_iterator i=notesInternals.categoriesMap()->begin();i!=notesInternals.categoriesMap()->end();++i)
        categoryPairList.push_back(*i);
    ui->categoriesComboBox->clear();
    int j=-1;
    for(unsigned int i=0;i<categoryPairList.size();++i)
    {
        ui->categoriesComboBox->addItem(NotesInternals::getCategoryEncrypted(categoryPairList[i])?QIcon("/icons/lock_add.png"):QIcon(),
                                        NotesInternals::getCategoryName(categoryPairList[i]));
        if(currentCategory==NotesInternals::getCategory(categoryPairList[i]))
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
    entryPairList.clear();
    ui->entriesListWidget->clear();
    if(currentCategory)
    {
        int j=-1;
        for(EntriesMap::const_iterator i=currentCategory->entriesMap()->cbegin();i!=currentCategory->entriesMap()->cend();++i)
        {
            if(NotesInternals::getEntryName(*i).startsWith(ui->entryFilterLineEdit->text(),Qt::CaseInsensitive))
                entryPairList.push_back(*i);
        }
        for(EntriesMap::const_iterator i=currentCategory->entriesMap()->cbegin();i!=currentCategory->entriesMap()->cend();++i)
        {
            if(!NotesInternals::getEntryName(*i).startsWith(ui->entryFilterLineEdit->text(),Qt::CaseInsensitive) && NotesInternals::getEntryName(*i).contains(ui->entryFilterLineEdit->text(),Qt::CaseInsensitive))
                entryPairList.push_back(*i);
        }
        for(unsigned int i=0;i<entryPairList.size();++i)
        {
            ui->entriesListWidget->addItem(NotesInternals::getEntryName(entryPairList[i]));
            if(currentEntry==NotesInternals::getEntry(entryPairList[i]))
                j=i;
        }
        if(j==-1)
        {
            currentEntryPair=EntryPair(NameDate(QString(""),QDateTime::fromMSecsSinceEpoch(0)),0);
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
        ui->entryTextEdit->setText(NotesInternals::getEntryText(currentEntryPair));
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
    selectCategoryPair(categoryPairList[index],true);
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
    if(i>=0 && i<entryPairList.size())
    {
        selectEntryPair(entryPairList[i]);
        updateEntryTextUI();
    }
}

void NotizenMainWindow::on_encryptionPushButton_clicked()
{

}
