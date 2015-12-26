#include "defaultvalues.h"

namespace DefaultValues
{
    QFont entryFont;
    QColor entryFontColor;
    QFont printingFontCategory;
    QFont printingFontEntry;
}

using namespace DefaultValues;

DefaultValuesInitializer::DefaultValuesInitializer()
{
    entryFont=QFont(QFont().defaultFamily(),10,QFont::Normal,false);
    entryFontColor=QColor(Qt::black);
    printingFontCategory=QFont(QFont().defaultFamily(),16,QFont::Bold,false);
    printingFontCategory.setUnderline(true);
    printingFontEntry=QFont(QFont().defaultFamily(),14,QFont::Bold,false);
    printingFontEntry.setUnderline(true);
}
