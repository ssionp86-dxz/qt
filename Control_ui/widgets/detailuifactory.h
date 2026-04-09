#ifndef DETAILUIFACTORY_H
#define DETAILUIFACTORY_H

#include <QString>

class QFrame;
class QLabel;
class QProgressBar;
class QPushButton;
class QWidget;

namespace DetailUiFactory {
QFrame *createCardFrame(QWidget *parent);
QWidget *createHeader(const QString &iconText,
                      const QString &title,
                      const QString &accentColor,
                      QWidget *parent);
QLabel *createBadgeLabel(const QString &text, QWidget *parent);
QWidget *createInfoRow(const QString &labelText,
                       const QString &valueText,
                       QLabel **valueOut,
                       QWidget *parent);
QPushButton *createActionButton(const QString &text, bool primary, QWidget *parent);
QProgressBar *createThinProgress(const QString &chunkColor, QWidget *parent);
QLabel *createStatePill(const QString &text,
                        const QString &bgColor,
                        const QString &borderColor,
                        QWidget *parent);
} // namespace DetailUiFactory

#endif // DETAILUIFACTORY_H
