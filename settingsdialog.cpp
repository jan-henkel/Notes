#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::showSettings(NotesInternals *notesInternals)
{
    notesInternals_=notesInternals;
    QSettings settings("settings.ini",QSettings::IniFormat,this);

    settings.beginGroup("entry");
    ui->fontFamilyComboBox->setCurrentFont(settings.value("fontfamily",DefaultValues::entryFont.family()).toString());
    ui->fontSizeSpinbox->setValue(settings.value("fontsize",DefaultValues::entryFont.pointSize()).toInt());
    ui->fontBoldToolButton->setChecked(settings.value("fontbold",DefaultValues::entryFont.bold()).toBool());
    ui->fontItalicToolButton->setChecked(settings.value("fontitalic",DefaultValues::entryFont.italic()).toBool());
    entryFontColor_=QColor(settings.value("fontcolor",DefaultValues::entryFontColor).toString());
    ui->fontColorPushButton->setStyleSheet(QString("background-color: %1").arg(entryFontColor_.name()));
    settings.endGroup();

    settings.beginGroup("printing");
    ui->printingFontCategoryComboBox->setCurrentFont(settings.value("category_heading_fontfamily",DefaultValues::printingFontCategory.family()).toString());
    ui->printingFontCategorySizeSpinBox->setValue(settings.value("category_heading_fontsize",DefaultValues::printingFontCategory.pointSize()).toInt());
    ui->printingFontCategoryBoldToolButton->setChecked(settings.value("category_heading_bold",DefaultValues::printingFontCategory.bold()).toBool());
    ui->printingFontCategoryItalicToolButton->setChecked(settings.value("category_heading_italic",DefaultValues::printingFontCategory.italic()).toBool());
    ui->printingFontCategoryUnderlineToolButton->setChecked(settings.value("category_heading_underline",DefaultValues::printingFontCategory.underline()).toBool());
    ui->printingFontEntryComboBox->setCurrentFont(settings.value("entry_heading_fontfamily",DefaultValues::printingFontEntry.family()).toString());
    ui->printingFontEntrySizeSpinBox->setValue(settings.value("entry_heading_fontsize",DefaultValues::printingFontEntry.pointSize()).toInt());
    ui->printingFontEntryBoldToolButton->setChecked(settings.value("entry_heading_bold",DefaultValues::printingFontEntry.bold()).toBool());
    ui->printingFontEntryItalicToolButton->setChecked(settings.value("entry_heading_italic",DefaultValues::printingFontEntry.italic()).toBool());
    ui->printingFontEntryUnderlineToolButton->setChecked(settings.value("entry_heading_underline",DefaultValues::printingFontEntry.underline()).toBool());
    settings.endGroup();

    ui->defaultCategoryComboBox->clear();
    ui->defaultCategoryComboBox->addItem("");
    for(CategorySet::const_iterator i=notesInternals_->categorySet()->cbegin();i!=notesInternals_->categorySet()->cend();++i)
        ui->defaultCategoryComboBox->addItem(notesInternals->getCategoryName(*i));

    settings.beginGroup("mainwindow");
    ui->defaultPositionComboBox->setCurrentIndex(settings.value("default_position",DefaultValues::mainWindowPosition).toInt());
    ui->defaultCategoryComboBox->setCurrentIndex(settings.value("default_category",DefaultValues::categoryIndex).toInt()+1);
    ui->uiFontFamilyComboBox->setCurrentFont(settings.value("ui_fontfamily",DefaultValues::uiFont.family()).toString());
    ui->uiFontBoldToolButton->setChecked(settings.value("ui_fontfamily",DefaultValues::uiFont.bold()).toBool());
    ui->uiFontItalicToolButton->setChecked(settings.value("ui_fontitalic",DefaultValues::uiFont.italic()).toBool());
    ui->entryListFontBoldToolButton->setChecked(settings.value("entrylist_fontbold",DefaultValues::entryListFontBold).toBool());
    ui->entryListFontItalicToolButton->setChecked(settings.value("entrylist_fontitalic",DefaultValues::entryListFontItalic).toBool());
    ui->labelFontFamilyComboBox->setCurrentFont(settings.value("label_fontfamily",DefaultValues::labelFont.family()).toString());
    ui->labelFontSizeSpinBox->setValue(settings.value("label_fontsize",DefaultValues::labelFont.pointSize()).toInt());
    ui->labelFontBoldToolButton->setChecked(settings.value("label_fontbold",DefaultValues::labelFont.bold()).toBool());
    ui->labelFontItalicToolButton->setChecked(settings.value("label_fontitalic",DefaultValues::labelFont.italic()).toBool());
    categoryLabelBackground_=QColor(settings.value("categorylabel_background",DefaultValues::labelCategoryBackgroundColor).toString());
    ui->categoryLabelBackgroundColorPushButton->setStyleSheet(QString("background-color: %1").arg(categoryLabelBackground_.name()));
    categoryLabelFontColor_=QColor(settings.value("categorylabel_foreground",DefaultValues::labelCategoryFontColor).toString());
    ui->categoryLabelFontColorPushButton->setStyleSheet(QString("background-color: %1").arg(categoryLabelFontColor_.name()));
    entryLabelBackground_=QColor(settings.value("entrylabel_background",DefaultValues::labelEntryBackgroundColor).toString());
    ui->entryLabelBackgroundColorPushButton->setStyleSheet(QString("background-color: %1").arg(entryLabelBackground_.name()));
    entryLabelFontColor_=QColor(settings.value("entrylabel_foreground",DefaultValues::labelEntryFontColor).toString());
    ui->entryLabelFontColorPushButton->setStyleSheet(QString("background-color: %1").arg(entryLabelFontColor_.name()));
    ui->alwaysOnTopCheckBox->setChecked(settings.value("default_window_on_top",DefaultValues::windowAlwaysOnTop).toBool());
    settings.endGroup();

    show();
}

void SettingsDialog::on_applyPushButton_clicked()
{
    QSettings settings("settings.ini",QSettings::IniFormat,this);
    settings.beginGroup("entry");
    settings.setValue("fontfamily",ui->fontFamilyComboBox->currentFont().family());
    settings.setValue("fontsize",ui->fontSizeSpinbox->value());
    settings.setValue("fontbold",ui->fontBoldToolButton->isChecked());
    settings.setValue("fontitalic",ui->fontItalicToolButton->isChecked());
    settings.setValue("fontcolor",entryFontColor_.name());
    settings.endGroup();

    settings.beginGroup("printing");
    settings.setValue("category_heading_fontfamily",ui->printingFontCategoryComboBox->currentFont().family());
    settings.setValue("category_heading_fontsize",ui->printingFontCategorySizeSpinBox->value());
    settings.setValue("category_heading_bold",ui->printingFontCategoryBoldToolButton->isChecked());
    settings.setValue("category_heading_italic",ui->printingFontCategoryItalicToolButton->isChecked());
    settings.setValue("category_heading_underline",ui->printingFontCategoryUnderlineToolButton->isChecked());
    settings.setValue("entry_heading_fontfamily",ui->printingFontEntryComboBox->currentFont().family());
    settings.setValue("entry_heading_fontsize",ui->printingFontEntrySizeSpinBox->value());
    settings.setValue("entry_heading_bold",ui->printingFontEntryBoldToolButton->isChecked());
    settings.setValue("entry_heading_italic",ui->printingFontEntryItalicToolButton->isChecked());
    settings.setValue("entry_heading_underline",ui->printingFontEntryUnderlineToolButton->isChecked());
    settings.endGroup();

    //if one of the checkboxes is ticked, update all currently loaded entries (encrypted ones are only affected if encryption is active)
    if(ui->fontFamilyApplyAllCheckBox->isChecked() || ui->fontSizeApplyAllCheckBox->isChecked() || ui->fontStyleApplyAllCheckBox->isChecked() || ui->fontColorApplyAllCheckBox->isChecked())
    {
        NotizenTextEdit edit(this);
        for(CategorySet::const_iterator i=notesInternals_->categorySet()->cbegin();i!=notesInternals_->categorySet()->cend();++i)
        {
            const Category *category=notesInternals_->getCategory(*i);
            for(EntrySet::const_iterator j=category->entrySet()->cbegin();j!=category->entrySet()->cend();++j)
            {
                edit.setHtml(notesInternals_->getEntryText(*j));
                edit.selectAll();
                if(ui->fontFamilyApplyAllCheckBox->isChecked())
                    edit.setFontFamily(ui->fontFamilyComboBox->currentFont().family());
                if(ui->fontSizeApplyAllCheckBox->isChecked())
                    edit.setFontPointSize(ui->fontSizeSpinbox->value());
                if(ui->fontStyleApplyAllCheckBox->isChecked())
                {
                    edit.setFontWeight(ui->fontBoldToolButton->isChecked()?QFont::Bold:QFont::Normal);
                    edit.setFontItalic(ui->fontItalicToolButton->isChecked());
                }
                if(ui->fontColorApplyAllCheckBox->isChecked())
                    edit.setTextColor(entryFontColor_);
                notesInternals_->modifyEntryText(*i,*j,edit.toHtml());
            }
        }
    }

    emit updateMainWindow();
}

void SettingsDialog::on_applyPushButton_2_clicked()
{
    QSettings settings("settings.ini",QSettings::IniFormat,this);
    settings.beginGroup("mainwindow");
    settings.setValue("default_position",ui->defaultPositionComboBox->currentIndex());
    settings.setValue("default_category",ui->defaultCategoryComboBox->currentIndex());
    settings.setValue("ui_fontfamily",ui->uiFontFamilyComboBox->currentFont());
    settings.setValue("ui_fontfamily",ui->uiFontBoldToolButton->isChecked());
    settings.setValue("ui_fontitalic",ui->uiFontItalicToolButton->isChecked());
    settings.setValue("entrylist_fontbold",ui->entryListFontBoldToolButton->isChecked());
    settings.setValue("entrylist_fontitalic",ui->entryListFontItalicToolButton->isChecked());
    settings.setValue("label_fontfamily",ui->labelFontFamilyComboBox->currentFont());
    settings.setValue("label_fontsize",ui->labelFontSizeSpinBox->value());
    settings.setValue("label_fontbold",ui->labelFontBoldToolButton->isChecked());
    settings.setValue("label_fontitalic",ui->labelFontItalicToolButton->isChecked());
    settings.setValue("categorylabel_background", categoryLabelBackground_);
    settings.setValue("categorylabel_foreground", categoryLabelFontColor_);
    settings.setValue("entrylabel_background", entryLabelBackground_);
    settings.setValue("entrylabel_foreground", entryLabelFontColor_);
    settings.setValue("default_window_on_top",ui->alwaysOnTopCheckBox->isChecked());
    settings.endGroup();
}
