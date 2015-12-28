#ifndef DEFAULTVALUES_H
#define DEFAULTVALUES_H

#include <QFont>
#include <QTextCharFormat>
#include <QColor>

namespace DefaultValues
{
    extern QFont entryFont;
    extern QColor entryFontColor;
    extern QFont printingFontCategory;
    extern QFont printingFontEntry;
    extern int mainWindowPosition;
    extern int categoryIndex;
    extern QFont uiFont;
    extern bool entryListFontBold;
    extern bool entryListFontItalic;
    extern QFont labelFont;
    extern QColor labelCategoryBackgroundColor;
    extern QColor labelCategoryFontColor;
    extern QColor labelEntryBackgroundColor;
    extern QColor labelEntryFontColor;
    extern bool windowAlwaysOnTop;
    extern int mainWindowWidth;
    extern int mainWindowHeight;
    extern bool confirmDelete;
    extern bool autoSaveOnLeavingEntry;
    extern bool applySearchFilterToEntryList;
}

class DefaultValuesInitializer
{
public:
    DefaultValuesInitializer();
    void initialize();
};

#endif // DEFAULTVALUES_H
