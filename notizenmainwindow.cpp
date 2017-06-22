#include "notizenmainwindow.h"
#include "ui_notizenmainwindow.h"
#include <vector>

NotizenMainWindow::NotizenMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::NotizenMainWindow),
    notesInternals(this),
    initialShow_(true),
    settingsDialog(new SettingsDialog(this))
    //saveEntryShortcut(QKeySequence(Qt::CTRL,Qt::Key_S),this,"SLOT(saveEntryShortcutTriggered())")
{
    ui->setupUi(this);
    //update UI to show categories and so on
    updateFlags|=(CategoryListChanged|EntryListContentChanged|EntrySelectionChanged);
    syncModelAndUI();

    //connect notification signals from notesInternals, to set update flags accordingly
    QObject::connect(&(this->notesInternals),SIGNAL(categoryListChanged()),this,SLOT(categoryListChanged()),Qt::DirectConnection);
    QObject::connect(&(this->notesInternals),SIGNAL(categorySelectionChanged()),this,SLOT(categorySelectionChanged()),Qt::DirectConnection);
    QObject::connect(&(this->notesInternals),SIGNAL(categoryContentChanged()),this,SLOT(categoryContentChanged()),Qt::DirectConnection);
    QObject::connect(&(this->notesInternals),SIGNAL(entrySelectionChanged()),this,SLOT(entrySelectionChanged()),Qt::DirectConnection);
    QObject::connect(&(this->notesInternals),SIGNAL(entryContentChanged()),this,SLOT(entryContentChanged()),Qt::DirectConnection);

    //set default char format. to do: read this from a config file instead of hardcoding
    defaultTextCharFormat=ui->entryTextEdit->currentCharFormat();

    //set up event filter for category combobox (as of now unnecessary), entry filter/search box and entry list
    ui->categoriesComboBox->installEventFilter(this);
    ui->entryFilterLineEdit->installEventFilter(this);
    ui->entriesListWidget->installEventFilter(this);
    this->installEventFilter(this);

    //set up ctrl+s shortcut to save entry changes
    //saveEntryShortcut.setKey();
    //saveEntryShortcut.setEnabled(true);
    //QObject::connect(&saveEntryShortcut,SIGNAL(activated()),this,SLOT(saveEntryShortcutTriggered()));

    QObject::connect(settingsDialog,SIGNAL(updateMainWindow()),this,SLOT(settingsDialogApply()));
    QObject::connect(settingsDialog,SIGNAL(changePassword()),this,SLOT(settingsChangePassword()));

    ui->categoriesComboBox->completer()->setCaseSensitivity(Qt::CaseSensitive);
}

NotizenMainWindow::~NotizenMainWindow()
{
    delete ui;
}

//high level functions to be called directly by user interactions.
//they take care of the interaction with the underlying model, as well as requesting the necessary UI updates
//they always refer to the currently selected/viewed category or entry

//functions for adding, removing and saving entries and categories
//corresponding members of notesInternals are called and the UI subsequently updated according to update flags set by the operation

void NotizenMainWindow::addCategory()
{
    saveChanges();
    if(ui->categoriesComboBox->currentText()=="")
        return;
    notesInternals.selectCategory(notesInternals.addCategory(ui->categoriesComboBox->currentText()));
    syncModelAndUI();
}

void NotizenMainWindow::addEntry()
{
    saveChanges();
    notesInternals.selectEntry(notesInternals.addEntryToCurrentCategory(ui->entryFilterLineEdit->text()));
    syncModelAndUI();
}

void NotizenMainWindow::removeCategory()
{
    if(confirmDelete)
        if(QMessageBox(QMessageBox::Question,tr("Delete category"),tr("Are you sure?"),QMessageBox::Yes | QMessageBox::No,this).exec()!=QMessageBox::Yes)
            return;
    notesInternals.removeCurrentCategory();
    syncModelAndUI();
}

void NotizenMainWindow::removeEntry()
{
    if(confirmDelete)
        if(QMessageBox(QMessageBox::Question,tr("Delete entry"),tr("Are you sure?"),QMessageBox::Yes | QMessageBox::No,this).exec()!=QMessageBox::Yes)
            return;
    int index=std::distance(entryList.begin(),std::find(entryList.begin(),entryList.end(),notesInternals.currentEntry()));

    if(index<(int)entryList.size()-1)
        ++index;
    else
        --index;
    notesInternals.removeCurrentEntry();
    if(index>=0)
        notesInternals.selectEntry(entryList[index]);
    syncModelAndUI();
}

void NotizenMainWindow::printCategory()
{
    if(QPrinterInfo::availablePrinters().empty())
        QMessageBox(QMessageBox::Warning,tr("Error"),tr("No printers found"),QMessageBox::Ok,this).exec();
    else
    {
        QPrinter printer;
        std::unique_ptr<QPrintDialog> printDialog(new QPrintDialog(&printer,this));
        NotizenTextEdit textEdit(this);
        if(printDialog->exec() == QDialog::Accepted && notesInternals.isValid(notesInternals.currentCategory()))
        {
            textEdit.moveCursor(QTextCursor::End);
            QTextBlockFormat f;
            f.setAlignment(Qt::AlignCenter);
            textEdit.setCurrentFont(printingFontCategory);
            textEdit.textCursor().insertBlock(f);
            textEdit.textCursor().insertText(notesInternals.currentCategory().name);
            f.setAlignment(Qt::AlignLeft);
            textEdit.textCursor().insertBlock(f);
            for(EntrySet::const_iterator i=NotesInternals::getContent(notesInternals.currentCategory())->entrySet()->cbegin();i!=NotesInternals::getContent(notesInternals.currentCategory())->entrySet()->cend();++i)
            {
                textEdit.setCurrentFont(printingFontEntry);
                QTextCharFormat f=textEdit.currentCharFormat();
                f.setForeground(Qt::black);
                textEdit.setCurrentCharFormat(f);
                textEdit.insertPlainText(QString("\n\n"));
                textEdit.insertPlainText((*i).name+QString("\n\n"));

                textEdit.setCurrentFont(DefaultValues::entryFont);
                textEdit.insertHtml(NotesInternals::getText(*i));
            }

            textEdit.print(&printer);
        }
    }
}

void NotizenMainWindow::printEntry()
{
    if(QPrinterInfo::availablePrinters().empty())
        QMessageBox(QMessageBox::Warning,tr("Error"),tr("No printers found"),QMessageBox::Ok,this).exec();
    else
    {
        QPrinter printer;
        std::unique_ptr<QPrintDialog> printDialog(new QPrintDialog(&printer,this));
        NotizenTextEdit textEdit(this);
        if(printDialog->exec() == QDialog::Accepted)
        {
            textEdit.setCurrentFont(printingFontEntry);
            textEdit.insertPlainText((notesInternals.currentEntry()).name+QString("\n\n"));

            textEdit.setCurrentFont(DefaultValues::entryFont);
            textEdit.insertHtml(NotesInternals::getText(notesInternals.currentEntry()));

            textEdit.print(&printer);
        }
    }
}

void NotizenMainWindow::saveEntry()
{
    notesInternals.modifyCurrentEntryText(ui->entryTextEdit->toHtml());
    //entry content is already as represented in the editor now, clear flag
    //(to do: implement something like model-view-viewmodel pattern cleanly to avoid this sort of thing)
    updateFlags&=~EntryContentChanged;
    //changes were saved, currently no unsaved changes
    setEdited(false);
    syncModelAndUI();
}

void NotizenMainWindow::renameCategory()
{
    QInputDialog renameDialog(this);
    setUpModalInputDialog(renameDialog,tr("Choose new title"),tr("Enter a new name for category '")+notesInternals.currentCategory().name+QString("'"),
                          QInputDialog::TextInput,notesInternals.currentCategory().name);
    renameDialog.exec();
    if(renameDialog.result()==QInputDialog::Accepted && renameDialog.textValue()!="")
    {
        notesInternals.renameCurrentCategory(renameDialog.textValue());
        syncModelAndUI();
    }
}

void NotizenMainWindow::renameEntry()
{
    saveChanges();
    QInputDialog renameDialog(this);
    setUpModalInputDialog(renameDialog,tr("Choose new title"),tr("Enter a new name for entry '")+(notesInternals.currentEntry()).name+QString("'"),
                          QInputDialog::TextInput,(notesInternals.currentEntry()).name);
    renameDialog.exec();
    if(renameDialog.result()==QInputDialog::Accepted && renameDialog.textValue()!="")
    {
        notesInternals.renameCurrentEntry(renameDialog.textValue());
        syncModelAndUI();
    }
}

void NotizenMainWindow::toggleCategoryEncryption()
{
    if(NotesInternals::isEncrypted(notesInternals.currentCategory()))
        if(QMessageBox(QMessageBox::Warning,tr("Are you sure?"),tr("Decrypting a category will cause its contents to be written on disk in plaintext. Proceed?"),QMessageBox::Yes|QMessageBox::No,this).exec()!=QMessageBox::Yes)
            return;
    notesInternals.toggleCurrentCategoryEncryption();
    syncModelAndUI();
}

void NotizenMainWindow::moveEntry(Category newCategory)
{
    notesInternals.moveCurrentEntry(newCategory);
    syncModelAndUI();
}

void NotizenMainWindow::selectCategory()
{
    saveChanges();
    int index=ui->categoriesComboBox->currentIndex();
    if(index>=0 && index<(int)categoryList.size())
    {
        updateFlags|=EntryListContentChanged;
        ui->entryFilterLineEdit->setText("");
        notesInternals.selectCategory(categoryList[index]);
        syncModelAndUI();
    }
    else
    {
        updateFlags|=EntryListContentChanged;
        ui->entryFilterLineEdit->setText("");
        notesInternals.selectCategory(notesInternals.invalidCategory());
        syncModelAndUI();
    }
}

void NotizenMainWindow::selectEntry()
{
    saveChanges();
    int i=ui->entriesListWidget->currentIndex().row();
    if(i>=0 && i<(int)entryList.size())
    {
        notesInternals.selectEntry(entryList[i]);
        syncModelAndUI();
    }
}

void NotizenMainWindow::toggleEncryption()
{
    saveChanges();
    if(!notesInternals.encryptionEnabled())
    {
        PasswordDialog dlg(this);
        dlg.setModal(true);
        QObject::connect(&dlg,SIGNAL(newPasswordSet(CryptoPP::SecByteBlock,bool)),this,SLOT(createNewPassword(CryptoPP::SecByteBlock,bool)));
        QObject::connect(&dlg,SIGNAL(passwordEntered(CryptoPP::SecByteBlock)),this,SLOT(passwordEntered(CryptoPP::SecByteBlock)));
        QObject::connect(&dlg,SIGNAL(passwordMismatch()),this,SLOT(passwordMismatch()));
        if(!notesInternals.masterKeyExists())
        {
            if(QMessageBox(QMessageBox::Question,tr("Master key not set"),tr("No master key set. Do you wish to create a new one?"),QMessageBox::Yes | QMessageBox::No,this).exec()==QMessageBox::Yes)
                dlg.showWithMode(PasswordDialog::CreateMasterKey);
        }
        else
        {
            dlg.showWithMode(PasswordDialog::AskPassword);
        }
    }
    else
    {
        notesInternals.disableEncryption();
        syncModelAndUI();
        ui->encryptionPushButton->setIcon(QIcon(":/icons/lock_add.png"));
    }
}

void NotizenMainWindow::openSettings()
{
    settingsDialog->showSettings(&this->notesInternals,&this->categoryList);
}

void NotizenMainWindow::showEvent(QShowEvent *e)
{
    QMainWindow::showEvent(e);
    if(initialShow_)
    {
        initialShow_=false;
        readSettings();
    }
}

//big function to handle UI updates as requested in the updateFlags variable

void NotizenMainWindow::syncModelAndUI()
{
    //set of visible categories has changed, update categoryList and subsequently the combobox widget
    if(updateFlags & CategoryListChanged)
    {
        categoryList.clear();
        //encrypted categories go first, unencrypted second
        for(CategorySet::const_iterator i=notesInternals.categorySet()->cbegin();i!=notesInternals.categorySet()->cend();++i)
        {
            if(NotesInternals::isEncrypted(*i))
                categoryList.push_back(*i);
        }
        for(CategorySet::const_iterator i=notesInternals.categorySet()->cbegin();i!=notesInternals.categorySet()->cend();++i)
        {
            if(!NotesInternals::isEncrypted(*i))
                categoryList.push_back(*i);
        }
        ui->categoriesComboBox->clear();
        int j=-1;
        //fill combobox with categories
        for(int i=0;i<(int)categoryList.size();++i)
        {
            //encrypted categories get a lock-icon
            ui->categoriesComboBox->addItem(NotesInternals::isEncrypted(categoryList[i])?QIcon(":/icons/lock.png"):QIcon(""),
                                            categoryList[i].name);
            //find index of current selection as specified by notesInternals (adapts to changes like renaming)
            if(notesInternals.currentCategory()==categoryList[i])
                j=i;
        }
        //select item corresponding to current category (-1 if current category is not set, i.e. invalid)
        ui->categoriesComboBox->setCurrentIndex(j);
        //category list was dealt with, remove flag
        updateFlags &= ~CategoryListChanged;
    }
    //selected category or set of entries matching the search filter has changed, update entryList and entry list widget accordingly
    if(updateFlags & (CategorySelectionChanged|EntryListContentChanged))
    {
        entryList.clear();
        ui->entriesListWidget->clear();
        if(notesInternals.isValid(notesInternals.currentCategory()))
        {
            if(applySearchFilterToEntryList)
            {
                //entries which start with searchfilter text go first
                for(EntrySet::const_iterator i=NotesInternals::getContent(notesInternals.currentCategory())->entrySet()->cbegin();i!=NotesInternals::getContent(notesInternals.currentCategory())->entrySet()->cend();++i)
                {
                    if((*i).name.startsWith(ui->entryFilterLineEdit->text(),Qt::CaseInsensitive))
                        entryList.push_back(*i);
                }
                //entries which don't begin with the searchfilter text but do otherwise match the filter go second
                for(EntrySet::const_iterator i=NotesInternals::getContent(notesInternals.currentCategory())->entrySet()->cbegin();i!=NotesInternals::getContent(notesInternals.currentCategory())->entrySet()->cend();++i)
                {
                    if(!(*i).name.startsWith(ui->entryFilterLineEdit->text(),Qt::CaseInsensitive) && (*i).name.contains(ui->entryFilterLineEdit->text(),Qt::CaseInsensitive))
                        entryList.push_back(*i);
                }
            }
            else
            {
                std::copy(NotesInternals::getContent(notesInternals.currentCategory())->entrySet()->cbegin(),NotesInternals::getContent(notesInternals.currentCategory())->entrySet()->cend(),std::back_inserter(entryList));
            }
            //entry list widget is filled with items corresponding to elements of entryList
            for(int i=0;i<(int)entryList.size();++i)
                ui->entriesListWidget->addItem((entryList[i]).name);
            //flag is not yet deleted, since entry selection wasn't dealt with
        }
    }
    //deal with entry selection if either of the preceding criteria apply or another entry was selected (also applies if the selected entry was renamed)
    if(updateFlags & (CategorySelectionChanged|EntryListContentChanged|EntrySelectionChanged))
    {
        int j=-1;
        //find entry list widget index associated with the entry currently selected (as specified by the notesInternals object)
        for(int i=0;i<(int)entryList.size();++i)
        {
            if(notesInternals.currentEntry()==entryList[i])
                j=i;
        }
        //if entry isn't currently represented in entryList (i.e. not visible), notesInternals must be adjusted to ensure that GUI and model agree
        if(j==-1)
            notesInternals.selectEntry(NotesInternals::invalidEntry());
        //select item corresponding to currently selected entry
        ui->entriesListWidget->setCurrentRow(j);
        //all effects of selecting a category and otherwise altering the listed entries are dealt with, remove associated flags
        updateFlags &= ~(CategorySelectionChanged|EntryListContentChanged);
    }
    //if an entry was selected or the content of the entry currently viewed was altered (e.g. by saving), contents of the text editor are updated
    if(updateFlags & (EntrySelectionChanged|EntryContentChanged))
    {
        //load appropriate content from notesInternals
        ui->entryTextEdit->setHtml(notesInternals.getText(notesInternals.currentEntry()));
        //remaining unsaved changes (if any) are void
        setEdited(false);
        //effects of altered entry content were dealt with, remove related flag
        updateFlags &= ~EntryContentChanged;
    }
    //as a final consequence of selecting an entry, reset text formatting to the default
    if(updateFlags & EntrySelectionChanged)
    {
        ui->entryTextEdit->setFont(entryFont);
        ui->entryTextEdit->setTextColor(entryFontColor);
        ui->entryTextEdit->setCurrentCharFormat(defaultTextCharFormat);
        //effects of selecting another entry were processed, remove flag
        updateFlags &= ~EntrySelectionChanged;
    }
}

//read settings from settings.ini and apply them
void NotizenMainWindow::readSettings()
{
    QSettings settings("settings.ini",QSettings::IniFormat,this);

    settings.beginGroup("entry");
    confirmDelete=settings.value("confirm_deletion",DefaultValues::confirmDelete).toBool();
    autoSaveOnLeavingEntry=settings.value("auto_save",DefaultValues::autoSaveOnLeavingEntry).toBool();
    applySearchFilterToEntryList=settings.value("apply_searchfilter",DefaultValues::applySearchFilterToEntryList).toBool();
    entryFont=QFont(settings.value("fontfamily",DefaultValues::entryFont.family()).toString(),
                    settings.value("fontsize",DefaultValues::entryFont.pointSize()).toInt(),
                    settings.value("fontbold",DefaultValues::entryFont.bold()).toBool()?QFont::Bold:QFont::Normal,
                    settings.value("fontitalic",DefaultValues::entryFont.italic()).toBool());
    entryFontColor=QColor(settings.value("fontcolor",DefaultValues::entryFontColor).toString());
    settings.endGroup();
    defaultTextCharFormat.setFont(entryFont);
    defaultTextCharFormat.setForeground(QBrush(entryFontColor));
    settings.beginGroup("printing");
    printingFontCategory=QFont(settings.value("category_heading_fontfamily",DefaultValues::printingFontCategory.family()).toString(),
                               settings.value("category_heading_fontsize",DefaultValues::printingFontCategory.pointSize()).toInt(),
                               settings.value("category_heading_bold",DefaultValues::printingFontCategory.bold()).toBool()?QFont::Bold:QFont::Normal,
                               settings.value("category_heading_italic",DefaultValues::printingFontCategory.italic()).toBool());
    printingFontCategory.setUnderline(settings.value("category_heading_underline",DefaultValues::printingFontCategory.underline()).toBool());
    printingFontEntry=QFont(settings.value("entry_heading_fontfamily",DefaultValues::printingFontEntry.family()).toString(),
                            settings.value("entry_heading_fontsize",DefaultValues::printingFontEntry.pointSize()).toInt(),
                            settings.value("entry_heading_bold",DefaultValues::printingFontEntry.bold()).toBool()?QFont::Bold:QFont::Normal,
                            settings.value("entry_heading_italic",DefaultValues::printingFontEntry.italic()).toBool());
    printingFontEntry.setUnderline(settings.value("entry_heading_underline",DefaultValues::printingFontEntry.underline()).toBool());
    settings.endGroup();

    settings.beginGroup("mainwindow");
    this->resize(settings.value("last_width",DefaultValues::mainWindowWidth).toInt(),settings.value("last_height",DefaultValues::mainWindowHeight).toInt());

    int screenWidth=QApplication::desktop()->availableGeometry().width();
    int screenHeight=QApplication::desktop()->availableGeometry().height();
    int windowWidth=this->frameGeometry().width();
    int windowHeight=this->frameGeometry().height();
    switch(settings.value("default_position",DefaultValues::mainWindowPosition).toInt())
    {
    case 0:
        //this->setGeometry(QStyle::alignedRect(Qt::LeftToRight,Qt::AlignTop | Qt::AlignLeft,this->size(),QApplication::desktop()->availableGeometry()));
        move(0,0);
        break;
    case 1:
        //this->setGeometry(QStyle::alignedRect(Qt::LeftToRight,Qt::AlignTop | Qt::AlignRight,this->size(),QApplication::desktop()->availableGeometry()));
        move(screenWidth-windowWidth,0);
        break;
    case 2:
        //this->setGeometry(QStyle::alignedRect(Qt::LeftToRight,Qt::AlignBottom | Qt::AlignLeft,this->size(),QApplication::desktop()->availableGeometry()));
        move(0,screenHeight-windowHeight);
        break;
    case 3:
        //this->setGeometry(QStyle::alignedRect(Qt::LeftToRight,Qt::AlignBottom | Qt::AlignRight,this->size(),QApplication::desktop()->availableGeometry()));
        move(screenWidth-windowWidth,screenHeight-windowHeight);
        break;
    case 4:
        this->setGeometry(QStyle::alignedRect(Qt::LeftToRight,Qt::AlignCenter,this->size(),QApplication::desktop()->availableGeometry()));
        break;
    }

    QString categoryName=settings.value("default_category_name",DefaultValues::categoryName).toString();
    QString categoryDateTime=settings.value("default_category_date_time",DefaultValues::categoryDateTime).toString();
    int j=-1;
    for(int i=0;i<(int)categoryList.size() && j==-1;++i)
    {
        if(categoryList[i].name==categoryName && categoryList[i].date.toString()==categoryDateTime)
            j=i;
    }
    ui->categoriesComboBox->setCurrentIndex(j);
    selectCategory();
    ui->categoriesComboBox->setFont(QFont(settings.value("ui_fontfamily",DefaultValues::uiFont.family()).toString(),
                                    settings.value("ui_fontsize",DefaultValues::uiFont.pointSize()).toInt(),
                                    settings.value("ui_fontbold",DefaultValues::uiFont.bold()).toBool()?QFont::Bold:QFont::Normal,
                                    settings.value("ui_fontitalic",DefaultValues::uiFont.italic()).toBool()));
    ui->entryFilterLineEdit->setFont(ui->categoriesComboBox->font());
    ui->entriesListWidget->setFont(QFont(settings.value("ui_fontfamily",DefaultValues::uiFont.family()).toString(),
                                         settings.value("ui_fontsize",DefaultValues::uiFont.pointSize()).toInt(),
                                         settings.value("entrylist_fontbold",DefaultValues::entryListFontBold).toBool()?QFont::Bold:QFont::Normal,
                                         settings.value("entrylist_fontitalic",DefaultValues::entryListFontItalic).toBool()));
    ui->categoryLabel->setFont(QFont(settings.value("label_fontfamily",DefaultValues::labelFont.family()).toString(),
                                     settings.value("label_fontsize",DefaultValues::labelFont.pointSize()).toInt(),
                                     settings.value("label_fontbold",DefaultValues::labelFont.bold()).toBool()?QFont::Bold:QFont::Normal,
                                     settings.value("label_fontitalic",DefaultValues::labelFont.italic()).toBool()));
    ui->entryLabel->setFont(ui->categoryLabel->font());
    ui->categoryLabel->setStyleSheet(QString("background-color: %1;\ncolor: %2").arg(settings.value("categorylabel_background",DefaultValues::labelCategoryBackgroundColor.name()).toString(),settings.value("categorylabel_foreground",DefaultValues::labelCategoryFontColor.name()).toString()));
    ui->addCategoryPushButton->setStyleSheet(ui->categoryLabel->styleSheet());
    ui->removeCategoryPushButton->setStyleSheet(ui->categoryLabel->styleSheet());
    ui->printCategoryPushButton->setStyleSheet(ui->categoryLabel->styleSheet());
    ui->entryLabel->setStyleSheet(QString("background-color: %1;\ncolor: %2").arg(settings.value("entrylabel_background",DefaultValues::labelEntryBackgroundColor.name()).toString(),settings.value("entrylabel_foreground",DefaultValues::labelEntryFontColor.name()).toString()));
    ui->addEntryPushButton->setStyleSheet(ui->entryLabel->styleSheet());
    ui->removeEntryPushButton->setStyleSheet(ui->entryLabel->styleSheet());
    ui->printEntryPushButton->setStyleSheet(ui->entryLabel->styleSheet());

    toggleStayOnTop(settings.value("default_window_on_top",DefaultValues::windowAlwaysOnTop).toBool());
    ui->stayOnTopToolButton->setChecked(settings.value("default_window_on_top",DefaultValues::windowAlwaysOnTop).toBool());
    settings.endGroup();
}

//various auxiliary functions for specific purposes

void NotizenMainWindow::editEntryTextURL()
{
    QTextCharFormat f=ui->entryTextEdit->textCursor().charFormat();

    //query to obtain new URL for currently selected text
    QInputDialog changeURLDialog(this);
    setUpModalInputDialog(changeURLDialog,tr("Edit URL"),tr("Set a new URL for the current selection:"),QInputDialog::TextInput,ui->entryTextEdit->textCursor().charFormat().anchorHref());
    changeURLDialog.exec();

    if(changeURLDialog.result()!=QInputDialog::Accepted)
        return;
    //no URL means no clickable link
    if(changeURLDialog.textValue()=="")
    {
        f.setAnchor(false);
        f.setAnchorHref("");
        //set char format for current selection
        ui->entryTextEdit->textCursor().setCharFormat(f);
    }
    //otherwise set the URL to the one gathered above, format the text to make clickable link obvious
    else
    {
        f.setAnchor(true);
        f.setAnchorHref(changeURLDialog.textValue());
        f.setFontUnderline(true);
        f.setForeground(Qt::blue);
        //set char format for current selection
        ui->entryTextEdit->textCursor().setCharFormat(f);
    }
    //may be unnecessary
    ui->entryTextEdit->setCurrentCharFormat(f);
}

void NotizenMainWindow::setEdited(bool edited)
{
    edited_=edited;
    if(edited)
        ui->savePushButton->setIcon(QIcon(":/icons/disk_red.png"));
    else
        ui->savePushButton->setIcon(QIcon(":/icons/disk.png"));
}

//prompts user to save or discard changes
void NotizenMainWindow::saveChanges()
{
    if(edited())
    {
        if(autoSaveOnLeavingEntry || QMessageBox(QMessageBox::Question,tr("Save changes"),tr("The current entry was edited. Do you wish to save the changes?"),QMessageBox::Yes|QMessageBox::No,this).exec()==QMessageBox::Yes)
            saveEntry();
    }
}

//turns Qt::WindowStaysOnTopHint on or off
void NotizenMainWindow::toggleStayOnTop(bool stayOnTop)
{
    if(stayOnTop)
    {
        //this->hide();
        this->setWindowFlags(this->windowFlags()|Qt::CustomizeWindowHint|Qt::WindowStaysOnTopHint);
        this->show();
    }
    else
    {
        //this->hide();
        this->setWindowFlags(this->windowFlags()&(~(Qt::CustomizeWindowHint|Qt::WindowStaysOnTopHint|Qt::WindowStaysOnBottomHint)));
        this->show();
    }
}

//event filter to deal with user input not caught by the signal-slot mechanism
bool NotizenMainWindow::eventFilter(QObject *target, QEvent *e)
{
    if(target==ui->entriesListWidget)
    {
        if(e->type()==QEvent::KeyRelease)
        {
            selectEntry();
            //delete entries when 'del' is pressed
            if(((QKeyEvent*)e)->key()==Qt::Key_Delete)
                removeEntry();
        }
    }
    //add category on pressing return, if there isn't one of the same name in the list
    if(target==ui->categoriesComboBox)
    {
        if(e->type()==QEvent::KeyRelease)
        {
            if(((QKeyEvent*)e)->text()=="\n" || ((QKeyEvent*)e)->text()=="\r")
            {
                bool nameExists=false;
                for(int i=0;i<(int)categoryList.size();++i)
                    if(categoryList[i].name==ui->categoriesComboBox->currentText())
                        nameExists=true;
                if(!nameExists)
                    addCategory();
            }
        }
    }
    //select search text when entry search box gains focus
    if(target==ui->entryFilterLineEdit)
    {
        static bool selected=false;
        if(e->type()==QEvent::FocusOut)
            selected=false;
        if(e->type()==QEvent::MouseButtonPress && !selected && ui->entryFilterLineEdit->selectionStart()!=-1)
            selected=true;
        if(e->type()==QEvent::MouseButtonRelease && !selected)
        {
            ui->entryFilterLineEdit->selectAll();
            selected=true;
        }
    }
    if(e->type()==QEvent::KeyPress && ((QKeyEvent*)e)->matches(QKeySequence::Save))
        saveEntry();

    e->accept();
    return false;
}

void NotizenMainWindow::closeEvent(QCloseEvent *e)
{
    Q_UNUSED(e)
    saveChanges();
    QSettings settings("settings.ini",QSettings::IniFormat);
    settings.beginGroup("mainwindow");
    settings.setValue("last_width",this->width());
    settings.setValue("last_height",this->height());
    settings.endGroup();
}

void NotizenMainWindow::setUpModalInputDialog(QInputDialog &inputDialog,QString windowTitle,QString labelText,QInputDialog::InputMode inputMode,QString textValue)
{
    inputDialog.setWindowFlags(inputDialog.windowFlags()& ~Qt::WindowContextHelpButtonHint);
    inputDialog.setWindowTitle(windowTitle);
    inputDialog.setLabelText(labelText);
    inputDialog.setInputMode(inputMode);
    inputDialog.setTextValue(textValue);
    inputDialog.setModal(true);
    inputDialog.setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    inputDialog.adjustSize();
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
    Q_UNUSED(index)
    selectCategory();
}

void NotizenMainWindow::on_entryFilterLineEdit_textEdited(const QString &arg1)
{
    Q_UNUSED(arg1)
    if(applySearchFilterToEntryList)
    {
        saveChanges();
        updateFlags|=EntryListContentChanged;
        //notesInternals.selectEntry(NotesInternals::invalidEntry()); //optional, depending on search behavior
        syncModelAndUI();
        if(!entryList.empty())
            notesInternals.selectEntry(entryList[0]);
    }
    else if(ui->entryFilterLineEdit->text()!="")
    {
        saveChanges();
        int j=-1;
        int k=-1;
        for(int i=0;i<(int)entryList.size() && j==-1;++i)
        {
            if((entryList[i]).name.startsWith(ui->entryFilterLineEdit->text(),Qt::CaseInsensitive))
                j=i;
            if((entryList[i]).name.contains(ui->entryFilterLineEdit->text(),Qt::CaseInsensitive) && k==-1)
                k=i;
        }
        j=std::max(j,k);
        if(j>=0)
            notesInternals.selectEntry(entryList[j]);
    }
    //else
    //    notesInternals.selectEntry(NotesInternals::invalidEntry());
    syncModelAndUI();
}

void NotizenMainWindow::createNewPassword(CryptoPP::SecByteBlock password, bool createMasterKey)
{
    if(createMasterKey)
    {
        notesInternals.createNewMasterKey(password);
        if(!notesInternals.encryptionEnabled())
        {
            if(!notesInternals.enableEncryption(password))
            {
                QMessageBox(QMessageBox::Warning,tr("Error"),tr("An error occured while setting up encryption."),QMessageBox::Ok,this).exec();
                return;
            }
            syncModelAndUI();
            ui->encryptionPushButton->setIcon(QIcon(":/icons/lock_delete.png"));
        }
    }
    else
    {
        notesInternals.createNewPassword(password);
    }
}

void NotizenMainWindow::passwordEntered(CryptoPP::SecByteBlock password)
{
    if(!notesInternals.enableEncryption(password))
    {
        QMessageBox(QMessageBox::Warning,tr("Wrong password"),tr("The password you entered is incorrect."),QMessageBox::Ok,this).exec();
        return;
    }
    syncModelAndUI();
    ui->encryptionPushButton->setIcon(QIcon(":/icons/lock_delete.png"));
}

void NotizenMainWindow::passwordMismatch()
{
    QMessageBox(QMessageBox::Warning,tr("Inputs don't match"),tr("The passwords you entered do not match."),QMessageBox::Ok,this).exec();
    return;
}

void NotizenMainWindow::settingsDialogApply()
{
    readSettings();
    syncModelAndUI();
}

void NotizenMainWindow::settingsChangePassword()
{
    PasswordDialog dlg(this);
    dlg.setModal(true);
    QObject::connect(&dlg,SIGNAL(newPasswordSet(CryptoPP::SecByteBlock,bool)),this,SLOT(createNewPassword(CryptoPP::SecByteBlock,bool)));
    QObject::connect(&dlg,SIGNAL(passwordEntered(CryptoPP::SecByteBlock)),this,SLOT(passwordEntered(CryptoPP::SecByteBlock)));
    QObject::connect(&dlg,SIGNAL(passwordMismatch()),this,SLOT(passwordMismatch()));
    if(!notesInternals.encryptionEnabled())
    {
        if(!notesInternals.masterKeyExists())
        {
            if(QMessageBox(QMessageBox::Question,tr("Master key not set"),tr("No master key set. Do you wish to create a new one?"),QMessageBox::Yes | QMessageBox::No,this).exec()==QMessageBox::Yes)
                dlg.showWithMode(PasswordDialog::CreateMasterKey);
            return;
        }
        else
        {
            dlg.showWithMode(PasswordDialog::AskPassword);
        }
    }
    if(!notesInternals.encryptionEnabled())
        return;
    dlg.showWithMode(PasswordDialog::ChangePassword);
}

void NotizenMainWindow::moveEntryMenu(QAction *action)
{
    moveEntry(categoryList[action->data().toInt()]);
}

void NotizenMainWindow::on_savePushButton_clicked()
{
    saveEntry();
}

void NotizenMainWindow::on_entriesListWidget_pressed(const QModelIndex &index)
{
    Q_UNUSED(index)
    selectEntry();
}

void NotizenMainWindow::on_removeEntryPushButton_clicked()
{
    removeEntry();
}

void NotizenMainWindow::on_printEntryPushButton_clicked()
{
    printEntry();
}

void NotizenMainWindow::on_removeCategoryPushButton_clicked()
{
    removeCategory();
}

void NotizenMainWindow::on_entryTextEdit_anchorClicked(const QUrl &arg1)
{
    QDesktopServices::openUrl(arg1);
}

void NotizenMainWindow::on_printCategoryPushButton_clicked()
{
    printCategory();
}

void NotizenMainWindow::on_fontComboBox_activated(const QString &arg1)
{
    ui->entryTextEdit->setFontFamily(arg1);
}

void NotizenMainWindow::on_makeLinkCheckBox_clicked(bool checked)
{
    Q_UNUSED(checked)
    QTextCharFormat f=ui->entryTextEdit->textCursor().charFormat();
    if(f.anchorHref()!="" || f.anchorName()!="" || f.isAnchor())
    {
        f.setAnchor(false);
        f.setAnchorHref("");
        f.setFontUnderline(false);
        ui->entryTextEdit->textCursor().setCharFormat(f);
    }
    else
    {
        f.setAnchor(true);
        f.setAnchorHref(ui->entryTextEdit->textCursor().selectedText());
        f.setFontUnderline(true);
        f.setForeground(Qt::blue);
        ui->entryTextEdit->textCursor().setCharFormat(f);
    }
    ui->entryTextEdit->setFocus();
    ui->entryTextEdit->setCurrentCharFormat(f);
}

void NotizenMainWindow::on_colorPushButton_clicked()
{
    QColorDialog dialog(this);
    dialog.setModal(true);
    dialog.setCurrentColor(ui->entryTextEdit->textColor());
    dialog.exec();
    QColor col=dialog.currentColor();
    ui->colorPushButton->setStyleSheet(QString("background-color: %1").arg(col.name()));
    ui->entryTextEdit->setTextColor(col);
    ui->entryTextEdit->setFocus();
}

void NotizenMainWindow::on_fontSizeSpinBox_editingFinished()
{
    ui->entryTextEdit->setFontPointSize(ui->fontSizeSpinBox->value());
}

void NotizenMainWindow::on_italicToolButton_clicked(bool checked)
{
    ui->entryTextEdit->setFontItalic(checked);
}

void NotizenMainWindow::on_underlineToolButton_clicked(bool checked)
{
    ui->entryTextEdit->setFontUnderline(checked);
}

void NotizenMainWindow::on_boldToolButton_clicked(bool checked)
{
    ui->entryTextEdit->setFontWeight(checked?QFont::Bold:QFont::Normal);
}

void NotizenMainWindow::on_entriesListWidget_customContextMenuRequested(const QPoint &pos)
{
    Q_UNUSED(pos)
    if(notesInternals.isValid(notesInternals.currentCategory(),notesInternals.currentEntry()))
    {
        QMenu entriesContextMenu(this);
        QMenu moveMenu(this);
        moveMenu.setTitle(tr("Move entry"));
        for(int i=0;i<(int)categoryList.size();++i)
            moveMenu.addAction(categoryList[i].name)->setData(i);
        QObject::connect(&moveMenu,SIGNAL(triggered(QAction*)),this,SLOT(moveEntryMenu(QAction*)));
        entriesContextMenu.addMenu(&moveMenu);
        entriesContextMenu.addAction(this->ui->actionRenameEntry);
        entriesContextMenu.addAction(this->ui->actionDeleteEntry);
        entriesContextMenu.exec(QCursor::pos());
    }
}

void NotizenMainWindow::on_encryptionPushButton_clicked()
{
    toggleEncryption();
}

void NotizenMainWindow::on_entryTextEdit_currentCharFormatChanged(const QTextCharFormat &f)
{
    ui->fontComboBox->setCurrentText(ui->entryTextEdit->fontFamily());
    if(f.anchorHref()!="" || f.anchorName()!="" || f.isAnchor())
        ui->makeLinkCheckBox->setChecked(true);
    else
        ui->makeLinkCheckBox->setChecked(false);
    ui->colorPushButton->setStyleSheet("background-color: "+ui->entryTextEdit->textColor().name());
    ui->fontSizeSpinBox->setValue(ui->entryTextEdit->fontPointSize());
    ui->italicToolButton->setChecked(ui->entryTextEdit->fontItalic());
    ui->boldToolButton->setChecked(ui->entryTextEdit->fontWeight()==QFont::Bold);
    ui->underlineToolButton->setChecked(ui->entryTextEdit->fontUnderline());
}

void NotizenMainWindow::on_categoriesComboBox_customContextMenuRequested(const QPoint &pos)
{
    Q_UNUSED(pos)
    std::unique_ptr<QMenu> categoriesContextMenu(ui->categoriesComboBox->lineEdit()->createStandardContextMenu());
    categoriesContextMenu->removeAction(categoriesContextMenu->actions()[0]);
    categoriesContextMenu->removeAction(categoriesContextMenu->actions()[0]);
    if(notesInternals.isValid(notesInternals.currentCategory()))
    {
        if(notesInternals.encryptionEnabled())
            categoriesContextMenu->insertAction(categoriesContextMenu->actions()[0],ui->actionCategoryToggleEncryption);
        categoriesContextMenu->insertAction(categoriesContextMenu->actions()[0],ui->actionDeleteCategory);
        categoriesContextMenu->insertAction(categoriesContextMenu->actions()[0],ui->actionRenameCategory);
    }
    categoriesContextMenu->exec(QCursor::pos());
}

void NotizenMainWindow::on_actionRenameCategory_triggered()
{
    renameCategory();
}

void NotizenMainWindow::on_actionDeleteCategory_triggered()
{
    removeCategory();
}


void NotizenMainWindow::on_actionCategoryToggleEncryption_triggered()
{
    toggleCategoryEncryption();
}

void NotizenMainWindow::on_entryFilterLineEdit_returnPressed()
{
    addEntry();
}

void NotizenMainWindow::on_stayOnTopToolButton_clicked(bool checked)
{

   toggleStayOnTop(checked);
}

void NotizenMainWindow::on_actionRenameEntry_triggered()
{
    renameEntry();
}

void NotizenMainWindow::on_actionDeleteEntry_triggered()
{
    removeEntry();
}

void NotizenMainWindow::on_entryTextEdit_customContextMenuRequested(const QPoint &pos)
{
    Q_UNUSED(pos)
    std::unique_ptr<QMenu> entryTextContextMenu(ui->entryTextEdit->createStandardContextMenu());
    if(!ui->entryTextEdit->textCursor().selection().isEmpty())
        entryTextContextMenu->addAction(ui->actionEditURL);
    entryTextContextMenu->exec(QCursor::pos());
    //delete entryTextContextMenu;
}

void NotizenMainWindow::on_actionEditURL_triggered()
{
    editEntryTextURL();
}

/*void NotizenMainWindow::saveEntryShortcutTriggered()
{
    saveEntry();
}*/

void NotizenMainWindow::on_entryTextEdit_textChanged()
{
    if(notesInternals.isValid(notesInternals.currentCategory(),notesInternals.currentEntry()))
        setEdited(true);
}

void NotizenMainWindow::on_settingsPushButton_clicked()
{
    openSettings();
}
