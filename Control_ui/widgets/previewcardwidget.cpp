#include "previewcardwidget.h"

#include "stylehelper.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPixmap>
#include <QStyle>
#include <QVBoxLayout>

PreviewCardWidget::PreviewCardWidget(const QString &moduleId,
                                     const QString &iconPath,
                                     const QString &title,
                                     const QString &meta,
                                     const QString &value,
                                     const QString &accentColor,
                                     QWidget *parent)
    : QFrame(parent)
    , m_moduleId(moduleId)
    , m_accentColor(accentColor)
    , m_active(false)
    , m_iconLabel(nullptr)
    , m_titleLabel(nullptr)
    , m_metaLabel(nullptr)
    , m_valueLabel(nullptr)
    , m_ledLabel(nullptr)
    , m_summaryFrame(nullptr)
{
    setObjectName("previewCard");
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setMinimumHeight(172);

    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(12, 8, 12, 8);
    rootLayout->setSpacing(6);

    QWidget *topRow = new QWidget(this);
    QHBoxLayout *topLayout = new QHBoxLayout(topRow);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(12);

    m_iconLabel = new QLabel(topRow);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    m_iconLabel->setFixedSize(42, 42);
    m_iconLabel->setObjectName("previewIcon");
    const QPixmap iconPixmap(iconPath);
    if (!iconPixmap.isNull()) {
        m_iconLabel->setPixmap(iconPixmap.scaled(m_iconLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    QWidget *titleBlock = new QWidget(topRow);
    QVBoxLayout *titleLayout = new QVBoxLayout(titleBlock);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(6);

    m_titleLabel = new QLabel(title, titleBlock);
    m_titleLabel->setObjectName("previewTitle");

    m_valueLabel = new QLabel(value, titleBlock);
    m_valueLabel->setObjectName("previewValue");

    titleLayout->addWidget(m_titleLabel);
    titleLayout->addWidget(m_valueLabel);

    m_ledLabel = new QLabel(topRow);
    m_ledLabel->setObjectName("previewLed");
    m_ledLabel->setFixedSize(12, 12);

    topLayout->addWidget(m_iconLabel, 0, Qt::AlignTop);
    topLayout->addWidget(titleBlock, 1);
    topLayout->addWidget(m_ledLabel, 0, Qt::AlignTop);

    m_summaryFrame = new QFrame(this);
    m_summaryFrame->setObjectName("previewSummaryFrame");
    QHBoxLayout *summaryLayout = new QHBoxLayout(m_summaryFrame);
    summaryLayout->setContentsMargins(0, 0, 0, 0);
    summaryLayout->setSpacing(0);

    m_metaLabel = new QLabel(meta, m_summaryFrame);
    m_metaLabel->setObjectName("previewMeta");
    m_metaLabel->setWordWrap(true);

    summaryLayout->addWidget(m_metaLabel, 1);

    rootLayout->addWidget(topRow);
    rootLayout->addWidget(m_summaryFrame);

    setLedState(LedOk);
    applyAccent();
    refreshStyle();
}

QString PreviewCardWidget::moduleId() const
{
    return m_moduleId;
}

void PreviewCardWidget::setActive(bool active)
{
    if (m_active == active) {
        return;
    }

    m_active = active;
    refreshStyle();
}

void PreviewCardWidget::setMetaText(const QString &text)
{
    m_metaLabel->setText(text);
}

void PreviewCardWidget::setValueText(const QString &text)
{
    m_valueLabel->setText(text);
}

void PreviewCardWidget::setLedState(PreviewCardWidget::LedState state)
{
    switch (state) {
    case LedOk:
        m_ledLabel->setStyleSheet("background:#22c55e; border-radius:6px;");
        break;
    case LedWarning:
        m_ledLabel->setStyleSheet("background:#f59e0b; border-radius:6px;");
        break;
    case LedOff:
        m_ledLabel->setStyleSheet("background:#64748b; border-radius:6px;");
        break;
    }
}

void PreviewCardWidget::mousePressEvent(QMouseEvent *event)
{
    emit clicked(m_moduleId);
    QFrame::mousePressEvent(event);
}

void PreviewCardWidget::refreshStyle()
{
    setProperty("active", m_active);
    setStyleSheet(StyleHelper::loadStyleSheet(QStringLiteral(":/styles/previewcard.qss")));
    style()->unpolish(this);
    style()->polish(this);
    update();
    applyAccent();
}

void PreviewCardWidget::applyAccent()
{
    if (!m_summaryFrame) {
        return;
    }

    m_summaryFrame->setStyleSheet(QString(
        "QFrame#previewSummaryFrame {"
        "background:transparent;"
        "border:none;"
        "}"
    ).arg(m_accentColor));
}
