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
}

class DefaultValuesInitializer
{
public:
    DefaultValuesInitializer();
};

#endif // DEFAULTVALUES_H
