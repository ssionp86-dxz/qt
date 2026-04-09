#include "stylehelper.h"

#include <QFile>

namespace StyleHelper {

QString loadStyleSheet(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    return QString::fromUtf8(file.readAll());
}

} // namespace StyleHelper
