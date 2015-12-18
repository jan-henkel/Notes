#include "notizenmainwindow.h"
#include "ui_notizenmainwindow.h"
#include <vector>

NotizenMainWindow::NotizenMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::NotizenMainWindow),
    notesInternals(this)
{
    ui->setupUi(this);
    updateTags|=(CategoryListChanged|EntryListContentChanged|EntrySelectionChanged);
    syncModelAndUI();

    QObject::connect(&(this->notesInternals),SIGNAL(categoryListChanged()),this,SLOT(categoryListChanged()),Qt::DirectConnection);
    QObject::connect(&(this->notesInternals),SIGNAL(categorySelectionChanged()),this,SLOT(categorySelectionChanged()),Qt::DirectConnection);
    QObject::connect(&(this->notesInternals),SIGNAL(categoryContentChanged()),this,SLOT(categoryContentChanged()),Qt::DirectConnection);
    QObject::connect(&(this->notesInternals),SIGNAL(entrySelectionChanged()),this,SLOT(entrySelectionChanged()),Qt::DirectConnection);
    QObject::connect(&(this->notesInternals),SIGNAL(entryContentChanged()),this,SLOT(entryContentChanged()),Qt::DirectConnection);
}

NotizenMainWindow::~NotizenMainWindow()
{
    delete ui;
}

void NotizenMainWindow::addCategory()
{
    notesInternals.selectCategory(notesInternals.addCategory(ui->categoriesComboBox->currentText()));
    syncModelAndUI();
}

void NotizenMainWindow::addEntry()
{
    notesInternals.selectEntry(notesInternals.addEntryToCurrentCategory(ui->entryFilterLineEdit->text()));
    syncModelAndUI();
}

void NotizenMainWindow::saveEntry()
{
    notesInternals.modifyCurrentEntryText(ui->entryTextEdit->toPlainText());
    syncModelAndUI();
}

void NotizenMainWindow::removeEntry()
{
    notesInternals.removeCurrentEntry();
    syncModelAndUI();
}

void NotizenMainWindow::syncModelAndUI()
{
    if(updateTags & CategoryListChanged)
    {
        categoryPairList.clear();
        std::copy(notesInternals.categoriesMap()->cbegin(),notesInternals.categoriesMap()->cend(),std::back_inserter(categoryPairList));
        ui->categoriesComboBox->clear();
        int j=-1;
        for(int i=0;i<(int)categoryPairList.size();++i)
        {
            ui->categoriesComboBox->addItem(NotesInternals::getCategoryEncrypted(categoryPairList[i])?QIcon("/icons/lock_add.png"):QIcon(),
                                            NotesInternals::getCategoryName(categoryPairList[i]));
            if(notesInternals.currentCategoryPair()==categoryPairList[i])
                j=i;
        }
        ui->categoriesComboBox->setCurrentIndex(j);
        updateTags &= ~CategoryListChanged;
    }
    if(updateTags & (CategorySelectionChanged|EntryListContentChanged))
    {
        entryPairList.clear();
        ui->entriesListWidget->clear();
        if(notesInternals.isValid(notesInternals.currentCategoryPair()))
        {
            for(EntriesMap::const_iterator i=NotesInternals::getCategory(notesInternals.currentCategoryPair())->entriesMap()->cbegin();i!=NotesInternals::getCategory(notesInternals.currentCategoryPair())->entriesMap()->cend();++i)
            {
                if(NotesInternals::getEntryName(*i).startsWith(ui->entryFilterLineEdit->text(),Qt::CaseInsensitive))
                    entryPairList.push_back(*i);
            }
            for(EntriesMap::const_iterator i=NotesInternals::getCategory(notesInternals.currentCategoryPair())->entriesMap()->cbegin();i!=NotesInternals::getCategory(notesInternals.currentCategoryPair())->entriesMap()->cend();++i)
            {
                if(!NotesInternals::getEntryName(*i).startsWith(ui->entryFilterLineEdit->text(),Qt::CaseInsensitive) && NotesInternals::getEntryName(*i).contains(ui->entryFilterLineEdit->text(),Qt::CaseInsensitive))
                    entryPairList.push_back(*i);
            }
            for(int i=0;i<(int)entryPairList.size();++i)
                ui->entriesListWidget->addItem(NotesInternals::getEntryName(entryPairList[i]));
        }
    }
    if(updateTags & (CategorySelectionChanged|EntryListContentChanged|EntrySelectionChanged))
    {
        int j=-1;
        for(int i=0;i<(int)entryPairList.size();++i)
        {
            if(notesInternals.currentEntryPair()==entryPairList[i])
                j=i;
        }
        ui->entriesListWidget->setCurrentRow(j);
        if(j==-1)
            notesInternals.selectEntry(NotesInternals::invalidEntryPair());
        updateTags &= ~(CategorySelectionChanged|EntryListContentChanged);
    }
    if(updateTags & (EntrySelectionChanged|EntryContentChanged))
    {
        ui->entryTextEdit->setText(notesInternals.getEntryText(notesInternals.currentEntryPair()));
        updateTags &= ~(EntrySelectionChanged|EntryContentChanged);
    }
}

void NotizenMainWindow::on_addCategoryPushButton_clicked()
{
    addCategory();
}

void NotizenMainWindow::on_addEntryPushButton_clicked()
{
    addEntry();
}

void NotizenMainWindow::on_categoriesComboBox_activated(int index)
{
    if(index>=0 && index<categoryPairList.size())
    {
        updateTags|=EntryListContentChanged;
        ui->entryFilterLineEdit->setText("");
        notesInternals.selectCategory(categoryPairList[index]);
        syncModelAndUI();
    }
}

void NotizenMainWindow::on_entriesListWidget_currentRowChanged(int currentRow)
{

}

void NotizenMainWindow::on_entryFilterLineEdit_textEdited(const QString &arg1)
{
    updateTags|=EntryListContentChanged;
    notesInternals.selectEntry(NotesInternals::invalidEntryPair()); //optional, depending on search behavior
    syncModelAndUI();
    if(!entryPairList.empty())
    {
        notesInternals.selectEntry(entryPairList[0]);
        syncModelAndUI();
    }
}

void NotizenMainWindow::on_savePushButton_clicked()
{
    saveEntry();
}

void NotizenMainWindow::on_entriesListWidget_pressed(const QModelIndex &index)
{
    int i=ui->entriesListWidget->currentIndex().row();
    if(i>=0 && i<(int)entryPairList.size())
    {
        notesInternals.selectEntry(entryPairList[i]);
        syncModelAndUI();
    }
}

void NotizenMainWindow::on_removeEntryPushButton_clicked()
{
    removeEntry();
}
