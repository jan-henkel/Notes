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

    QObject::connect(&(this->notesInternals),SIGNAL(categoryListChanged()),this,SLOT(categoryListChanged()),Qt::DirectConnection);
    QObject::connect(&(this->notesInternals),SIGNAL(categoryChanged()),this,SLOT(categoryChanged()),Qt::DirectConnection);
    QObject::connect(&(this->notesInternals),SIGNAL(entryChanged()),this,SLOT(entryChanged()),Qt::DirectConnection);
}

NotizenMainWindow::~NotizenMainWindow()
{
    delete ui;
}

void NotizenMainWindow::addCategory()
{
    notesInternals.selectCategory(notesInternals.addCategory(ui->categoriesComboBox->currentText()));
    selectCategoryPair(notesInternals.addCategory(categoryName),true);
    updateListsAndUI();
}

void NotizenMainWindow::addEntry(QString entryName)
{
    if(NotesInternals::getCategory(currentCategoryPair))
        selectEntryPair(notesInternals.addEntry(currentCategoryPair,entryName));
    updateEntryListAndUI();
    updateEntryTextUI();
}

void NotizenMainWindow::saveEntry()
{
    if(NotesInternals::getCategory(currentCategoryPair) && NotesInternals::getEntry(currentEntryPair))
    {
        notesInternals.modifyEntryText(currentCategoryPair,currentEntryPair,ui->entryTextEdit->toPlainText());
    }
}

void NotizenMainWindow::selectCategoryPair(CategoryPair newCategoryPair, bool resetEntry)
{
    currentCategoryPair=newCategoryPair;
    if(resetEntry)
    {
        currentEntryPair=NotesInternals::invalidEntryPair();
    }
}

void NotizenMainWindow::selectEntryPair(EntryPair newEntryPair)
{
    currentEntryPair=newEntryPair;
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
        if(NotesInternals::getCategory(currentCategoryPair)==NotesInternals::getCategory(categoryPairList[i]))
            j=i;
    }
    if(j==-1)
    {
        currentCategoryPair=NotesInternals::invalidCategoryPair();
    }
    ui->categoriesComboBox->setCurrentIndex(j);
}

void NotizenMainWindow::updateEntryListAndUI()
{
    entryPairList.clear();
    ui->entriesListWidget->clear();
    if(NotesInternals::getCategory(currentCategoryPair))
    {
        int j=-1;
        for(EntriesMap::const_iterator i=NotesInternals::getCategory(currentCategoryPair)->entriesMap()->cbegin();i!=NotesInternals::getCategory(currentCategoryPair)->entriesMap()->cend();++i)
        {
            if(NotesInternals::getEntryName(*i).startsWith(ui->entryFilterLineEdit->text(),Qt::CaseInsensitive))
                entryPairList.push_back(*i);
        }
        for(EntriesMap::const_iterator i=NotesInternals::getCategory(currentCategoryPair)->entriesMap()->cbegin();i!=NotesInternals::getCategory(currentCategoryPair)->entriesMap()->cend();++i)
        {
            if(!NotesInternals::getEntryName(*i).startsWith(ui->entryFilterLineEdit->text(),Qt::CaseInsensitive) && NotesInternals::getEntryName(*i).contains(ui->entryFilterLineEdit->text(),Qt::CaseInsensitive))
                entryPairList.push_back(*i);
        }
        for(unsigned int i=0;i<entryPairList.size();++i)
        {
            ui->entriesListWidget->addItem(NotesInternals::getEntryName(entryPairList[i]));
            if(NotesInternals::getEntry(currentEntryPair)==NotesInternals::getEntry(entryPairList[i]))
                j=i;
        }
        if(j==-1)
        {
            currentEntryPair=NotesInternals::invalidEntryPair();
            updateEntryTextUI();
        }
        ui->entriesListWidget->setCurrentRow(j);
    }
}

void NotizenMainWindow::updateEntryTextUI()
{
    ui->entryTextEdit->clear();
    if(NotesInternals::getCategory(currentCategoryPair) && NotesInternals::getEntry(currentEntryPair))
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
        updateTags &= !CategoryListChanged;
    }
    if(updateTags & EntryListChanged)
    {
        entryPairList.clear();
        ui->categoriesComboBox->clear();
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
            int j=-1;
            for(int i=0;i<(int)entryPairList.size();++i)
            {
                ui->entriesListWidget->addItem(NotesInternals::getEntryName(entryPairList[i]));
                if(notesInternals.currentEntryPair()==entryPairList[i])
                    j=i;
            }
            ui->entriesListWidget->setCurrentRow(j);
            if(j==-1)
                notesInternals.selectEntry(NotesInternals::invalidEntryPair());
            updateTags &= !EntryListChanged;
        }
    }
    if(updateTags & EntryTextChanged)
    {
        ui->entryTextEdit->setText(notesInternals.getEntryText(notesInternals.currentEntryPair()));
    }
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
