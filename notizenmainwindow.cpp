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
    //ui->categoriesComboBox->installEventFilter(this);
}

NotizenMainWindow::~NotizenMainWindow()
{
    delete ui;
}

//functions for adding, removing and saving entries and categories
//corresponding members of notesInternals are called and the UI subsequently updated according to update flags set by the operation

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
    notesInternals.modifyCurrentEntryText(ui->entryTextEdit->toHtml());
    syncModelAndUI();
}

void NotizenMainWindow::removeEntry()
{
    notesInternals.removeCurrentEntry();
    syncModelAndUI();
}

void NotizenMainWindow::removeCategory()
{
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
        QTextEdit textEdit;
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
        QTextEdit textEdit;
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
        QString pw=QInputDialog::getText(this,"PW","PW",QLineEdit::Password);
        if(!notesInternals.enableEncryption(QCA::SecureArray(pw.toUtf8())))
        {
            QMessageBox msg(this);
            msg.setText("Wrong pw");
            msg.setModal(true);
            msg.exec();
        }
        else
        {
            syncModelAndUI();
            ui->encryptionPushButton->setIcon(QIcon(":/icons/lock_delete.png"));
        }
    }
    else
    {
        notesInternals.disableEncryption();
        syncModelAndUI();
        ui->encryptionPushButton->setIcon(QIcon(":/icons/lock_add.png"));
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
        ui->categoriesComboBox->clear();
        int j=-1;
        for(int i=0;i<(int)categoryPairList.size();++i)
        {
            ui->categoriesComboBox->addItem(NotesInternals::getCategoryEncrypted(categoryPairList[i])?QIcon(":/icons/lock_add.png"):QIcon(""),
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
        ui->entriesListWidget->setCurrentRow(j);
        if(j==-1)
            notesInternals.selectEntry(NotesInternals::invalidEntryPair());
        updateFlags &= ~(CategorySelectionChanged|EntryListContentChanged);
    }
    if(updateFlags & (EntrySelectionChanged|EntryContentChanged))
    {
        ui->entryTextEdit->setHtml(notesInternals.getEntryText(notesInternals.currentEntryPair()));
        ui->entryTextEdit->setCurrentCharFormat(defaultTextCharFormat);
        updateFlags &= ~(EntrySelectionChanged|EntryContentChanged);
    }
}

/*bool NotizenMainWindow::eventFilter(QObject *target, QEvent *e)
{
    if(target==this->ui->categoriesComboBox->view() || target==this->ui->categoriesComboBox)
    {
        if(e->type()==QEvent::ContextMenu)
        {
            //e->ignore();
            e->accept();
            return true;
        }
    }
    e->accept();
    return false;
}*/

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
    if(index>=0 && index<(int)categoryPairList.size())
    {
        updateFlags|=EntryListContentChanged;
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
    updateFlags|=EntryListContentChanged;
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

void NotizenMainWindow::on_entryTextEdit_cursorPositionChanged()
{
    //QTextCharFormat f=ui->entryTextEdit->textCursor().charFormat();
    //f.setAnchor(false);
    //f.setAnchorHref("");
    //ui->entryTextEdit->textCursor().setCharFormat(f);
    //ui->entryTextEdit->textCursor().setBlockCharFormat(f);
    /*QTextCharFormat f=ui->entryTextEdit->textCursor().charFormat();
    ui->fontComboBox->setCurrentText(f.fontFamily());
    if(f.anchorHref()!="" || f.anchorName()!="" || f.isAnchor())
        ui->makeLinkCheckBox->setChecked(true);
    else
        ui->makeLinkCheckBox->setChecked(false);
    ui->colorPushButton->setStyleSheet("background-color: "+f.foreground().color().name());
    ui->fontSizeSpinBox->setValue(f.font().pointSize());
    ui->italicToolButton->setChecked(f.font().italic());
    ui->boldToolButton->setChecked(f.font().bold());
    ui->underlineToolButton->setChecked(f.font().underline());*/
}


    /*QString tmp=ui->entryTextEdit->textCursor().selectedText().toHtmlEscaped();
    ui->entryTextEdit->textCursor().insertHtml(QString("<a href='")+tmp+QString("'>")+tmp+QString("</a>"));*/



void NotizenMainWindow::on_entryTextEdit_textChanged()
{
    /*QTextCharFormat f=ui->entryTextEdit->currentCharFormat();
    f.setAnchor(false);
    f.setAnchorName("");
    f.setAnchorHref("");
    f.setFontUnderline(false);
    f.clearForeground();
    ui->entryTextEdit->setCurrentCharFormat(f);*/
    /*QTextCursor textCursor=QTextCursor(ui->entryTextEdit->textCursor());
    if(!textCursor.atStart() && textCursor.charFormat()!=f)
    {
        textCursor.setPosition(ui->entryTextEdit->textCursor().position()-1);
        textCursor.movePosition(QTextCursor::Right,QTextCursor::KeepAnchor);
        textCursor.setCharFormat(f);
    }*/

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

void NotizenMainWindow::on_fontSizeSpinBox_valueChanged(int arg1)
{
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
