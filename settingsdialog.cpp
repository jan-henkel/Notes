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
