#include "detailuifactory.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QSizePolicy>
#include <QVBoxLayout>

namespace DetailUiFactory {


QFrame *createCardFrame(QWidget *parent)
{
    QFrame *card = new QFrame(parent);
    card->setObjectName("detailCard");
    return card;
}

QWidget *createHeader(const QString &iconText, const QString &title, const QString &accentColor, QWidget *parent)
{
    QWidget *header = new QWidget(parent);
    QVBoxLayout *wrapperLayout = new QVBoxLayout(header);
    wrapperLayout->setContentsMargins(0, 0, 0, 0);
    wrapperLayout->setSpacing(14);

    QWidget *topRow = new QWidget(header);
    QHBoxLayout *topLayout = new QHBoxLayout(topRow);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(12);

    QLabel *iconLabel = new QLabel(iconText, topRow);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setFixedSize(44, 44);
    iconLabel->setStyleSheet(QString(
        "background:transparent;"
        "color:%1;"
        "border:none;"
        "font-size:28px;"
        "font-weight:700;"
    ).arg(accentColor));

    QLabel *titleLabel = new QLabel(title, topRow);
    titleLabel->setObjectName("detailTitle");

    topLayout->addWidget(iconLabel);
    topLayout->addWidget(titleLabel);
    topLayout->addStretch();

    QFrame *line = new QFrame(header);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("background:#243144; max-height:1px; border:none;");

    wrapperLayout->addWidget(topRow);
    wrapperLayout->addWidget(line);
    return header;
}

QLabel *createBadgeLabel(const QString &text, QWidget *parent)
{
    QLabel *valueLabel = new QLabel(text, parent);
    valueLabel->setObjectName("valueBadge");
    valueLabel->setAlignment(Qt::AlignCenter);
    valueLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    return valueLabel;
}

QWidget *createInfoRow(const QString &labelText, const QString &valueText, QLabel **valueOut, QWidget *parent)
{
    QWidget *row = new QWidget(parent);
    QHBoxLayout *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    QLabel *label = new QLabel(labelText, row);
    label->setObjectName("metricLabel");

    QLabel *value = createBadgeLabel(valueText, row);

    layout->addWidget(label);
    layout->addStretch();
    layout->addWidget(value);

    if (valueOut) {
        *valueOut = value;
    }

    return row;
}

QPushButton *createActionButton(const QString &text, bool primary, QWidget *parent)
{
    QPushButton *button = new QPushButton(text, parent);
    button->setCursor(Qt::PointingHandCursor);
    button->setMinimumHeight(38);
    button->setObjectName(primary ? "primaryActionButton" : "secondaryActionButton");
    return button;
}

QProgressBar *createThinProgress(const QString &chunkColor, QWidget *parent)
{
    QProgressBar *bar = new QProgressBar(parent);
    bar->setRange(0, 100);
    bar->setTextVisible(false);
    bar->setFixedHeight(10);
    bar->setStyleSheet(QString(
        "QProgressBar {"
        " background:#26344f;"
        " border:none;"
        " border-radius:5px;"
        "}"
        "QProgressBar::chunk {"
        " background:%1;"
        " border-radius:5px;"
        "}"
    ).arg(chunkColor));
    return bar;
}

QLabel *createStatePill(const QString &text, const QString &bgColor, const QString &borderColor, QWidget *parent)
{
    QLabel *label = new QLabel(text, parent);
    label->setAlignment(Qt::AlignCenter);
    label->setMinimumWidth(84);
    label->setStyleSheet(QString(
        "background:%1;"
        "color:#ffffff;"
        "border:1px solid %2;"
        "border-radius:16px;"
        "padding:6px 12px;"
        "font-weight:700;"
    ).arg(bgColor, borderColor));
    return label;
}
} // namespace DetailUiFactory
