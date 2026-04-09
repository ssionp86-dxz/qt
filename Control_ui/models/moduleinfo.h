#ifndef MODULEINFO_H
#define MODULEINFO_H

#include <QString>

struct ModuleInfo
{
    QString id;
    QString iconPath;
    QString title;
    QString meta;
    QString value;
    QString accentColor;
    bool warning;
};

#endif // MODULEINFO_H
