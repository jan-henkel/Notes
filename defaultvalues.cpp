#include "defaultvalues.h"

namespace DefaultValues
{
    QFont entryFont;
    QColor entryFontColor;
    QFont printingFontCategory;
    QFont printingFontEntry;
    int mainWindowPosition;
    int categoryIndex;
    QFont uiFont;
    bool entryListFontBold;
    bool entryListFontItalic;
    QFont labelFont;
    QColor labelCategoryBackgroundColor;
    QColor labelCategoryFontColor;
    QColor labelEntryBackgroundColor;
    QColor labelEntryFontColor;
    bool windowAlwaysOnTop;
}

using namespace DefaultValues;

DefaultValuesInitializer::DefaultValuesInitializer()
{

}

void DefaultValuesInitializer::initialize()
{
    entryFont=QFont(QFont().defaultFamily(),10,QFont::Normal,false);
    entryFontColor=QColor(Qt::black);
    printingFontCategory=QFont(QFont().defaultFamily(),16,QFont::Bold,false);
    printingFontCategory.setUnderline(true);
    printingFontEntry=QFont(QFont().defaultFamily(),14,QFont::Bold,false);
    printingFontEntry.setUnderline(true);
    mainWindowPosition=0;
    categoryIndex=-1;
    uiFont=QFont(QFont().defaultFamily(),10,QFont::Bold,false);
    entryListFontBold=false;
    entryListFontItalic=false;
    labelFont=QFont(QFont().defaultFamily(),12,QFont::Normal,false);
    labelCategoryBackgroundColor=QColor(255, 189, 155);
    labelCategoryFontColor=QColor(Qt::black);
    labelEntryBackgroundColor=QColor(207, 228, 255);
    labelEntryFontColor=QColor(Qt::black);
    windowAlwaysOnTop=true;
}