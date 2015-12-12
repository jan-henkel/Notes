#include "notizenmainwindow.h"
#include "ui_notizenmainwindow.h"
#include <vector>

NotizenMainWindow::NotizenMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::NotizenMainWindow),
    notesInternals(this)
{
    ui->setupUi(this);
    //update UI
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
    defaultTextCharFormat.setFont(QFont("Trebuchet MS",10,QFont::Normal,false));
    defaultTextCharFormat.setForeground(QBrush(Qt::black));

    //ui->categoriesComboBox->view()->installEventFilter(this);
    ui->categoriesComboBox->installEventFilter(this);
    ui->entriesListWidget->installEventFilter(this);
}

NotizenMainWindow::~NotizenMainWindow()
{
    delete ui;
}

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

void NotizenMainWindow::saveEntry()
{
    notesInternals.modifyCurrentEntryText(ui->entryTextEdit->toHtml());
    syncModelAndUI();
}

void NotizenMainWindow::removeEntry()
{
    if(QMessageBox(QMessageBox::Question,tr("Delete entry"),tr("Are you sure?"),QMessageBox::Yes | QMessageBox::No,this).exec()!=QMessageBox::Yes)
        return;
    notesInternals.removeCurrentEntry();
    syncModelAndUI();
}

void NotizenMainWindow::removeCategory()
{
    if(QMessageBox(QMessageBox::Question,tr("Delete category"),tr("Are you sure?"),QMessageBox::Yes | QMessageBox::No,this).exec()!=QMessageBox::Yes)
        return;
    notesInternals.removeCurrentCategory();
    syncModelAndUI();
}

void NotizenMainWindow::printEntry()
{
    if(QPrinterInfo::availablePrinters().empty())
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Error");
        msgBox.setText(tr("No printers found"));
        msgBox.exec();
    }
    else
    {
        QPrinter printer;
        QPrintDialog *printDialog=new QPrintDialog(&printer,this);
        NotizenTextEdit textEdit(this);
        if(printDialog->exec() == QDialog::Accepted)
        {
            textEdit.setCurrentFont(QFont("Trebuchet MS",16,QFont::Normal,false));
            textEdit.setFontUnderline(true);
            textEdit.insertPlainText(NotesInternals::getEntryName(notesInternals.currentEntryPair())+QString("\n\n"));

            textEdit.setCurrentFont(QFont("Trebuchet MS",14,QFont::Normal,false));
            textEdit.setFontUnderline(false);
            textEdit.insertHtml(NotesInternals::getEntryText(notesInternals.currentEntryPair()));

            textEdit.print(&printer);
        }
    }
}

void NotizenMainWindow::printCategory()
{
    if(QPrinterInfo::availablePrinters().empty())
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Error");
        msgBox.setText(tr("No printers found"));
        msgBox.exec();
    }
    else
    {
        QPrinter printer;
        QPrintDialog *printDialog=new QPrintDialog(&printer,this);
        NotizenTextEdit textEdit(this);
        if(printDialog->exec() == QDialog::Accepted && notesInternals.isValid(notesInternals.currentCategoryPair()))
        {
            textEdit.moveCursor(QTextCursor::End);
            QTextBlockFormat f;
            f.setAlignment(Qt::AlignCenter);
            textEdit.setCurrentFont(QFont("Trebuchet MS",16,QFont::Bold,false));
            textEdit.setFontUnderline(true);
            textEdit.textCursor().insertBlock(f);
            textEdit.textCursor().insertText(NotesInternals::getCategoryName(notesInternals.currentCategoryPair()));
            f.setAlignment(Qt::AlignLeft);
            textEdit.textCursor().insertBlock(f);
            for(EntrySet::const_iterator i=NotesInternals::getCategory(notesInternals.currentCategoryPair())->entrySet()->cbegin();i!=NotesInternals::getCategory(notesInternals.currentCategoryPair())->entrySet()->cend();++i)
            {
                textEdit.setCurrentFont(QFont("Trebuchet MS",14,QFont::Normal,false));
                textEdit.setFontUnderline(true);
                QTextCharFormat f=textEdit.currentCharFormat();
                f.setForeground(Qt::black);
                textEdit.setCurrentCharFormat(f);
                textEdit.insertPlainText(QString("\n\n"));
                textEdit.insertPlainText(NotesInternals::getEntryName(*i)+QString("\n\n"));

                textEdit.setCurrentFont(QFont("Trebuchet MS",12,QFont::Normal,false));
                textEdit.setFontUnderline(false);
                textEdit.insertHtml(NotesInternals::getEntryText(*i));
            }

            textEdit.print(&printer);
        }
    }
}

void NotizenMainWindow::toggleEncryption()
{
    if(!notesInternals.encryptionEnabled())
    {
        PasswordDialog dlg(this);
        dlg.setModal(true);
        QObject::connect(&dlg,SIGNAL(newPasswordSet(QCA::SecureArray,bool)),this,SLOT(createNewPassword(QCA::SecureArray,bool)));
        QObject::connect(&dlg,SIGNAL(passwordEntered(QCA::SecureArray)),this,SLOT(passwordEntered(QCA::SecureArray)));
        QObject::connect(&dlg,SIGNAL(passwordMismatch()),this,SLOT(passwordMismatch()));
        if(!notesInternals.masterKeyExists())
        {
            QMessageBox msg(this);
            msg.setModal(true);
            msg.setText("No master key set. Do you wish to create a new one?");
            msg.setWindowTitle(tr("Master key not set"));
            msg.setIcon(QMessageBox::Question);
            msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msg.exec();
            if(msg.result()==QMessageBox::Yes)
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

void NotizenMainWindow::renameCategory()
{
    QInputDialog renameDialog(this);
    renameDialog.setInputMode(QInputDialog::TextInput);
    renameDialog.setWindowFlags(renameDialog.windowFlags()& ~Qt::WindowContextHelpButtonHint);
    renameDialog.setWindowTitle(tr("Choose new title"));
    renameDialog.setLabelText(tr("Enter a new name for category '")+notesInternals.getCategoryName(notesInternals.currentCategoryPair())+QString("'"));
    renameDialog.setTextValue(notesInternals.getCategoryName(notesInternals.currentCategoryPair()));
    renameDialog.setModal(true);
    renameDialog.setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    renameDialog.adjustSize();
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
    renameDialog.setInputMode(QInputDialog::TextInput);
    renameDialog.setWindowFlags(renameDialog.windowFlags()& ~Qt::WindowContextHelpButtonHint);
    renameDialog.setWindowTitle(tr("Choose new title"));
    renameDialog.setLabelText(tr("Enter a new name for entry '")+notesInternals.getEntryName(notesInternals.currentEntryPair())+QString("'"));
    renameDialog.setTextValue(notesInternals.getEntryName(notesInternals.currentEntryPair()));
    renameDialog.setModal(true);
    renameDialog.setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    renameDialog.adjustSize();
    renameDialog.exec();
    if(renameDialog.result()==QInputDialog::Accepted && renameDialog.textValue()!="")
    {
        notesInternals.renameCurrentEntry(renameDialog.textValue());
        syncModelAndUI();
    }
}

void NotizenMainWindow::selectEntry()
{
    saveChanges();
    int i=ui->entriesListWidget->currentIndex().row();
    if(i>=0 && i<(int)entryPairList.size())
    {
        notesInternals.selectEntry(entryPairList[i]);
        syncModelAndUI();
    }
}

void NotizenMainWindow::selectCategory()
{
    saveChanges();
    int index=ui->categoriesComboBox->currentIndex();
    if(index>=0 && index<(int)categoryPairList.size())
    {
        updateFlags|=EntryListContentChanged;
        ui->entryFilterLineEdit->setText("");
        notesInternals.selectCategory(categoryPairList[index]);
        syncModelAndUI();
    }
}

void NotizenMainWindow::toggleStayOnTop(bool stayOnTop)
{
    if(stayOnTop)
    {
        this->hide();
        this->setWindowFlags(this->windowFlags()|Qt::CustomizeWindowHint|Qt::WindowStaysOnTopHint);
        this->show();
    }
    else
    {
        this->hide();
        this->setWindowFlags(this->windowFlags()&(~(Qt::CustomizeWindowHint|Qt::WindowStaysOnTopHint|Qt::WindowStaysOnBottomHint)));
        this->show();
    }
}

void NotizenMainWindow::editEntryTextURL()
{
    QTextCharFormat f=ui->entryTextEdit->textCursor().charFormat();

    QInputDialog changeURLDialog(this);
    changeURLDialog.setInputMode(QInputDialog::TextInput);
    changeURLDialog.setWindowFlags(changeURLDialog.windowFlags()& ~Qt::WindowContextHelpButtonHint);
    changeURLDialog.setWindowTitle(tr("Edit URL"));
    changeURLDialog.setLabelText(tr("Set a new URL for the current selection:"));
    changeURLDialog.setTextValue(ui->entryTextEdit->textCursor().charFormat().anchorHref());
    changeURLDialog.setModal(true);
    changeURLDialog.setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    changeURLDialog.adjustSize();
    changeURLDialog.exec();

    if(changeURLDialog.result()!=QInputDialog::Accepted)
    {
        return;
    }

    if(changeURLDialog.textValue()=="")
    {
        f.setAnchor(false);
        f.setAnchorName("");
        f.setAnchorHref("");
        ui->entryTextEdit->textCursor().setCharFormat(f);
    }
    else
    {
        f.setAnchor(true);
        f.setAnchorName(ui->entryTextEdit->textCursor().selectedText());
        f.setAnchorHref(changeURLDialog.textValue());
        //f.setFontUnderline(true);
        //f.setForeground(Qt::blue);
        ui->entryTextEdit->textCursor().setCharFormat(f);
    }
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

void NotizenMainWindow::saveChanges()
{
    if(edited())
    {
        if(QMessageBox(QMessageBox::Question,tr("Save changes"),tr("The current entry was edited. Do you wish to save the changes?"),QMessageBox::Yes|QMessageBox::No,this).exec()==QMessageBox::Yes)
            saveEntry();
    }
}

void NotizenMainWindow::syncModelAndUI()
{
    if(updateFlags & CategoryListChanged)
    {
        categoryPairList.clear();
        //std::copy(notesInternals.categoriesMap()->cbegin(),notesInternals.categoriesMap()->cend(),std::back_inserter(categoryPairList));
        for(CategorySet::const_iterator i=notesInternals.categorySet()->cbegin();i!=notesInternals.categorySet()->cend();++i)
        {
            if(NotesInternals::getCategoryEncrypted(*i))
                categoryPairList.push_back(*i);
        }
        for(CategorySet::const_iterator i=notesInternals.categorySet()->cbegin();i!=notesInternals.categorySet()->cend();++i)
        {
            if(!NotesInternals::getCategoryEncrypted(*i))
                categoryPairList.push_back(*i);
        }
        //std::function<bool(CategoryPair)> fObject1(NotesInternals::getCategoryEncrypted);
        //std::function<bool(CategoryPair)> fObject2(std::not1(fObject1));
        //std::copy_if(notesInternals.categorySet()->cbegin(),notesInternals.categorySet()->cend(),categoryPairList.begin(),NotesInternals::getCategoryEncrypted);
        //std::copy_if(notesInternals.categorySet()->cbegin(),notesInternals.categorySet()->cend(),categoryPairList.begin(),fObject2);

        ui->categoriesComboBox->clear();
        int j=-1;
        for(int i=0;i<(int)categoryPairList.size();++i)
        {
            ui->categoriesComboBox->addItem(NotesInternals::getCategoryEncrypted(categoryPairList[i])?QIcon(":/icons/lock.png"):QIcon(""),
                                            NotesInternals::getCategoryName(categoryPairList[i]));
            if(notesInternals.currentCategoryPair()==categoryPairList[i])
                j=i;
        }
        ui->categoriesComboBox->setCurrentIndex(j);
        updateFlags &= ~CategoryListChanged;
    }
    if(updateFlags & (CategorySelectionChanged|EntryListContentChanged))
    {
        entryPairList.clear();
        ui->entriesListWidget->clear();
        if(notesInternals.isValid(notesInternals.currentCategoryPair()))
        {
            for(EntrySet::const_iterator i=NotesInternals::getCategory(notesInternals.currentCategoryPair())->entrySet()->cbegin();i!=NotesInternals::getCategory(notesInternals.currentCategoryPair())->entrySet()->cend();++i)
            {
                if(NotesInternals::getEntryName(*i).startsWith(ui->entryFilterLineEdit->text(),Qt::CaseInsensitive))
                    entryPairList.push_back(*i);
            }
            for(EntrySet::const_iterator i=NotesInternals::getCategory(notesInternals.currentCategoryPair())->entrySet()->cbegin();i!=NotesInternals::getCategory(notesInternals.currentCategoryPair())->entrySet()->cend();++i)
            {
                if(!NotesInternals::getEntryName(*i).startsWith(ui->entryFilterLineEdit->text(),Qt::CaseInsensitive) && NotesInternals::getEntryName(*i).contains(ui->entryFilterLineEdit->text(),Qt::CaseInsensitive))
                    entryPairList.push_back(*i);
            }
            for(int i=0;i<(int)entryPairList.size();++i)
                ui->entriesListWidget->addItem(NotesInternals::getEntryName(entryPairList[i]));
        }
    }
    if(updateFlags & (CategorySelectionChanged|EntryListContentChanged|EntrySelectionChanged))
    {
        int j=-1;
        for(int i=0;i<(int)entryPairList.size();++i)
        {
            if(notesInternals.currentEntryPair()==entryPairList[i])
                j=i;
        }
        if(j==-1)
            notesInternals.selectEntry(NotesInternals::invalidEntryPair());
        ui->entriesListWidget->setCurrentRow(j);
        updateFlags &= ~(CategorySelectionChanged|EntryListContentChanged);
    }
    if(updateFlags & (EntrySelectionChanged|EntryContentChanged))
    {
        ui->entryTextEdit->setHtml(notesInternals.getEntryText(notesInternals.currentEntryPair()));
        setEdited(false);
        updateFlags &= ~EntryContentChanged;
    }
    if(updateFlags & EntrySelectionChanged)
    {
        ui->entryTextEdit->setCurrentCharFormat(defaultTextCharFormat);
        updateFlags &= ~EntrySelectionChanged;
    }
}

bool NotizenMainWindow::eventFilter(QObject *target, QEvent *e)
{
    if(target==ui->entriesListWidget)
    {
        if(e->type()==QEvent::KeyRelease)
        {
            selectEntry();
            if(((QKeyEvent*)e)->key()==Qt::Key_Delete)
                removeEntry();
        }
    }
    e->accept();
    return false;
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
    selectCategory();
}

void NotizenMainWindow::on_entriesListWidget_currentRowChanged(int currentRow)
{

}

void NotizenMainWindow::on_entryFilterLineEdit_textEdited(const QString &arg1)
{
    updateFlags|=EntryListContentChanged;
    notesInternals.selectEntry(NotesInternals::invalidEntryPair()); //optional, depending on search behavior
    syncModelAndUI();
    if(!entryPairList.empty())
    {
        notesInternals.selectEntry(entryPairList[0]);
        syncModelAndUI();
    }
}

void NotizenMainWindow::createNewPassword(QCA::SecureArray password, bool createMasterKey)
{
    if(createMasterKey)
    {
        notesInternals.createNewMasterKey(password);
        if(!notesInternals.enableEncryption(password))
        {
            QMessageBox(QMessageBox::Warning,tr("Error"),tr("An error occured while setting up encryption."),QMessageBox::Ok,this).exec();
            return;
        }
        syncModelAndUI();
        ui->encryptionPushButton->setIcon(QIcon(":/icons/lock_delete.png"));
    }
}

void NotizenMainWindow::passwordEntered(QCA::SecureArray password)
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

void NotizenMainWindow::moveEntryMenu(QAction *action)
{
    notesInternals.moveCurrentEntry(categoryPairList[action->data().toInt()]);
    syncModelAndUI();
}

void NotizenMainWindow::on_savePushButton_clicked()
{
    saveEntry();
}

void NotizenMainWindow::on_entriesListWidget_pressed(const QModelIndex &index)
{
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
    QTextCharFormat f=ui->entryTextEdit->currentCharFormat();
    f.setFontFamily(arg1);
    ui->entryTextEdit->setCurrentCharFormat(f);
}

void NotizenMainWindow::on_makeLinkCheckBox_clicked(bool checked)
{
    QTextCharFormat f=ui->entryTextEdit->textCursor().charFormat();
    if(f.anchorHref()!="" || f.anchorName()!="" || f.isAnchor())
    {
        f.setAnchor(false);
        f.setAnchorName("");
        f.setAnchorHref("");
        f.setFontUnderline(false);
        ui->entryTextEdit->textCursor().setCharFormat(f);
    }
    else
    {
        f.setAnchor(true);
        f.setAnchorName(ui->entryTextEdit->textCursor().selectedText());
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
    if(notesInternals.isValid(notesInternals.currentCategoryPair(),notesInternals.currentEntryPair()))
    {
        QMenu entriesContextMenu(this);
        QMenu moveMenu(this);
        moveMenu.setTitle(tr("Move entry"));
        for(int i=0;i<categoryPairList.size();++i)
            moveMenu.addAction(notesInternals.getCategoryName(categoryPairList[i]))->setData(i);
        QObject::connect(&moveMenu,SIGNAL(triggered(QAction*)),this,SLOT(moveEntryMenu(QAction*)));
        entriesContextMenu.addMenu(&moveMenu);
        entriesContextMenu.addAction(this->ui->actionRenameEntry);
        entriesContextMenu.exec(QCursor::pos());
    }
}

void NotizenMainWindow::on_encryptionPushButton_clicked()
{
    toggleEncryption();
}

void NotizenMainWindow::on_entryTextEdit_currentCharFormatChanged(const QTextCharFormat &f)
{
    ui->fontComboBox->setCurrentText(f.fontFamily());
    if(f.anchorHref()!="" || f.anchorName()!="" || f.isAnchor())
        ui->makeLinkCheckBox->setChecked(true);
    else
        ui->makeLinkCheckBox->setChecked(false);
    ui->colorPushButton->setStyleSheet("background-color: "+f.foreground().color().name());
    ui->fontSizeSpinBox->setValue(f.font().pointSize());
    ui->italicToolButton->setChecked(f.font().italic());
    ui->boldToolButton->setChecked(f.font().bold());
    ui->underlineToolButton->setChecked(f.font().underline());
}

void NotizenMainWindow::on_categoriesComboBox_customContextMenuRequested(const QPoint &pos)
{
    QMenu *categoriesContextMenu=ui->categoriesComboBox->lineEdit()->createStandardContextMenu();
    categoriesContextMenu->removeAction(categoriesContextMenu->actions()[0]);
    categoriesContextMenu->removeAction(categoriesContextMenu->actions()[0]);
    if(notesInternals.isValid(notesInternals.currentCategoryPair()))
        categoriesContextMenu->insertAction(categoriesContextMenu->actions()[0],ui->actionRenameCategory);
    categoriesContextMenu->exec(QCursor::pos());
    delete categoriesContextMenu;
}

void NotizenMainWindow::on_actionRenameCategory_triggered()
{
    renameCategory();
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

void NotizenMainWindow::on_entryTextEdit_customContextMenuRequested(const QPoint &pos)
{
    QMenu *entryTextContextMenu=ui->entryTextEdit->createStandardContextMenu();
    if(!ui->entryTextEdit->textCursor().selection().isEmpty())
        entryTextContextMenu->addAction(ui->actionEditURL);
    entryTextContextMenu->exec(QCursor::pos());
    delete entryTextContextMenu;
}

void NotizenMainWindow::on_actionEditURL_triggered()
{
    editEntryTextURL();
}

void NotizenMainWindow::on_entryTextEdit_textChanged()
{
    if(notesInternals.isValid(notesInternals.currentCategoryPair(),notesInternals.currentEntryPair()))
        setEdited(true);
}
