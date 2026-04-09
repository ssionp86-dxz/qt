#include "offsetmeasurementpage.h"

#include "detailuifactory.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QColor>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QRandomGenerator>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>
#include <QtMath>

using namespace DetailUiFactory;

namespace {

static QWidget *createMetricBlock(const QString &labelText,
                                  const QString &valueText,
                                  QLabel **valueLabel,
                                  QWidget *parent)
{
    QFrame *block = new QFrame(parent);
    block->setObjectName("offsetMetricBlock");

    QVBoxLayout *layout = new QVBoxLayout(block);
    layout->setContentsMargins(18, 12, 18, 12);
    layout->setSpacing(6);

    QLabel *label = new QLabel(labelText, block);
    label->setObjectName("offsetMetricLabel");

    QLabel *value = new QLabel(valueText, block);
    value->setObjectName("offsetMetricValue");

    layout->addWidget(label);
    layout->addWidget(value);

    if (valueLabel) {
        *valueLabel = value;
    }
    return block;
}

static int enabledChannelCount(const QVector<bool> &channels)
{
    int count = 0;
    for (bool enabled : channels) {
        if (enabled) {
            ++count;
        }
    }
    return count;
}

static QColor channelColor(int index)
{
    static const QColor colors[] = {
        QColor("#8b5cf6"),
        QColor("#22c55e"),
        QColor("#38bdf8"),
        QColor("#f59e0b"),
        QColor("#ef4444")
    };
    return colors[index % (sizeof(colors) / sizeof(colors[0]))];
}

static void setCenterItem(QTableWidget *table, int row, int col, const QString &text)
{
    QTableWidgetItem *item = table->item(row, col);
    if (!item) {
        item = new QTableWidgetItem;
        item->setTextAlignment(Qt::AlignCenter);
        table->setItem(row, col, item);
    }
    item->setText(text);
}

class ClickableCheckCellWidget : public QWidget
{
public:
    explicit ClickableCheckCellWidget(QWidget *parent = nullptr)
        : QWidget(parent)
        , m_checkBox(new QCheckBox(this))
    {
        m_checkBox->hide();
        m_checkBox->setCursor(Qt::PointingHandCursor);
        setCursor(Qt::PointingHandCursor);
        setMinimumSize(48, 40);
        connect(m_checkBox, &QCheckBox::toggled, this, [this](bool) {
            update();
        });
    }

    QCheckBox *checkBox() const
    {
        return m_checkBox;
    }

    bool isChecked() const
    {
        return m_checkBox->isChecked();
    }

    void setChecked(bool checked)
    {
        m_checkBox->setChecked(checked);
        update();
    }

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton && m_checkBox->isEnabled()) {
            m_checkBox->toggle();
            event->accept();
            return;
        }
        QWidget::mousePressEvent(event);
    }

    void paintEvent(QPaintEvent *event) override
    {
        QWidget::paintEvent(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const QRectF boxRect((width() - 20) / 2.0,
                             (height() - 20) / 2.0,
                             20.0,
                             20.0);

        const QColor borderColor = m_checkBox->isChecked()
                                       ? QColor(QStringLiteral("#d9e7ff"))
                                       : QColor(QStringLiteral("#d8dee9"));
        const QColor fillColor = QColor(QStringLiteral("#ffffff"));

        painter.setPen(QPen(borderColor, 1.0));
        painter.setBrush(fillColor);
        painter.drawRect(boxRect);

        if (m_checkBox->isChecked()) {
            QPainterPath tickPath;
            tickPath.moveTo(boxRect.left() + 4.5, boxRect.top() + 10.5);
            tickPath.lineTo(boxRect.left() + 8.5, boxRect.bottom() - 4.5);
            tickPath.lineTo(boxRect.right() - 4.0, boxRect.top() + 4.5);
            painter.setPen(QPen(QColor(QStringLiteral("#111827")), 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter.drawPath(tickPath);
        }
    }

private:
    QCheckBox *m_checkBox;
};

} // namespace

class TrendChartWidget : public QWidget
{
public:
    explicit TrendChartWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setMinimumHeight(360);
    }

    void setChannelSamples(const QVector<QVector<double> > &samples, const QVector<bool> &enabledChannels)
    {
        m_channelSamples = samples;
        m_enabledChannels = enabledChannels;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.fillRect(rect(), QColor("#0d1524"));

        const QRect chartRect = rect().adjusted(20, 26, -18, -40);

        painter.setPen(QColor("#24334d"));
        for (int i = 0; i < 6; ++i) {
            painter.drawLine(chartRect.left(),
                             chartRect.top() + i * chartRect.height() / 5,
                             chartRect.right(),
                             chartRect.top() + i * chartRect.height() / 5);
        }
        for (int i = 0; i < 13; ++i) {
            painter.drawLine(chartRect.left() + i * chartRect.width() / 12,
                             chartRect.top(),
                             chartRect.left() + i * chartRect.width() / 12,
                             chartRect.bottom());
        }

        painter.setPen(QColor("#7c3aed"));
        painter.drawText(24, 26, QString::fromUtf8("通道偏差趋势（ns）"));

        int activeWithSamples = 0;
        int maxPoints = 0;
        double maxValue = 20.0;
        for (int i = 0; i < m_channelSamples.size(); ++i) {
            if (!m_enabledChannels.value(i, false) || m_channelSamples.at(i).size() < 2) {
                continue;
            }
            ++activeWithSamples;
            maxPoints = qMax(maxPoints, m_channelSamples.at(i).size());
            for (double value : m_channelSamples.at(i)) {
                maxValue = qMax(maxValue, qAbs(value) + 5.0);
            }
        }

        if (activeWithSamples == 0) {
            painter.setPen(QColor("#94a3b8"));
            painter.drawText(chartRect, Qt::AlignCenter, QString::fromUtf8("请启用通道并点击“开始测量”"));
            return;
        }

        for (int channel = 0; channel < m_channelSamples.size(); ++channel) {
            const QVector<double> &samples = m_channelSamples.at(channel);
            if (!m_enabledChannels.value(channel, false) || samples.size() < 2) {
                continue;
            }

            QPainterPath path;
            for (int i = 0; i < samples.size(); ++i) {
                const double ratioX = static_cast<double>(i) / qMax(1, samples.size() - 1);
                const double ratioY = (samples.at(i) + maxValue) / (2.0 * maxValue);
                const QPointF point(chartRect.left() + ratioX * chartRect.width(),
                                    chartRect.bottom() - ratioY * chartRect.height());
                if (i == 0) {
                    path.moveTo(point);
                } else {
                    path.lineTo(point);
                }
            }

            painter.setPen(QPen(channelColor(channel), 2.0));
            painter.drawPath(path);
        }

        int legendX = chartRect.left();
        const int legendY = rect().bottom() - 18;
        for (int channel = 0; channel < m_enabledChannels.size(); ++channel) {
            if (!m_enabledChannels.value(channel, false)) {
                continue;
            }

            painter.setPen(QPen(channelColor(channel), 2.0));
            painter.drawLine(legendX, legendY, legendX + 18, legendY);
            painter.setPen(QColor("#cbd5e1"));
            painter.drawText(legendX + 24, legendY + 5, QString::fromUtf8("CH%1").arg(channel + 1));
            legendX += 66;
        }

        painter.setPen(QColor("#cbd5e1"));
        painter.drawText(chartRect.adjusted(0, 0, 0, 18),
                         Qt::AlignBottom | Qt::AlignRight,
                         QString::fromUtf8("最近 %1 组采样").arg(maxPoints));
    }

private:
    QVector<QVector<double> > m_channelSamples;
    QVector<bool> m_enabledChannels;
};

OffsetMeasurementPage::OffsetMeasurementPage(QWidget *parent)
    : QWidget(parent)
    , m_continuousMode(false)
    , m_measuring(false)
    , m_elapsedSeconds(0)
    , m_deltaValue(nullptr)
    , m_sigmaValue(nullptr)
    , m_modeValue(nullptr)
    , m_durationValue(nullptr)
    , m_stateValue(nullptr)
    , m_deltaBar(nullptr)
    , m_startButton(nullptr)
    , m_repeatButton(nullptr)
    , m_selectAllButton(nullptr)
    , m_enableHeaderWidget(nullptr)
    , m_enableHeaderLabel(nullptr)
    , m_singleRadio(nullptr)
    , m_continuousRadio(nullptr)
    , m_channelTable(nullptr)
    , m_trendChart(nullptr)
    , m_measureTimer(new QTimer(this))
{
    setObjectName("offsetMeasurementPage");

    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    QFrame *card = createCardFrame(this);
    rootLayout->addWidget(card);

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);

    m_channelEnabled = QVector<bool>() << false << false << false << false << false;
    m_lastSamples = QVector<double>() << 0.0 << 0.0 << 0.0 << 0.0 << 0.0;
    m_channelHistorySamples = QVector<QVector<double> >(5);

    QWidget *topSection = new QWidget(card);
    topSection->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    QHBoxLayout *topLayout = new QHBoxLayout(topSection);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(10);

    QGroupBox *leftPanel = new QGroupBox(QString(), topSection);
    leftPanel->setObjectName("sectionBox");
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(18, 18, 18, 18);
    leftLayout->setSpacing(14);

    QWidget *metricRow = new QWidget(leftPanel);
    QGridLayout *metricLayout = new QGridLayout(metricRow);
    metricLayout->setContentsMargins(0, 0, 0, 0);
    metricLayout->setHorizontalSpacing(16);
    metricLayout->setVerticalSpacing(16);
    metricLayout->addWidget(createMetricBlock(QString::fromUtf8("平均时差"), QString::fromUtf8("0.0 ns"), &m_deltaValue, metricRow), 0, 0);
    metricLayout->addWidget(createMetricBlock(QString::fromUtf8("平均稳定度"), QString::fromUtf8("0.0 ppb"), &m_sigmaValue, metricRow), 0, 1);
    metricLayout->addWidget(createMetricBlock(QString::fromUtf8("测量模式"), QString::fromUtf8("单次测量"), &m_modeValue, metricRow), 1, 0);
    metricLayout->addWidget(createMetricBlock(QString::fromUtf8("测量持续时间"), QString::fromUtf8("0 s"), &m_durationValue, metricRow), 1, 1);
    metricLayout->addWidget(createMetricBlock(QString::fromUtf8("状态"), QString::fromUtf8("未开始"), &m_stateValue, metricRow), 2, 0, 1, 2);
    metricLayout->setColumnStretch(0, 1);
    metricLayout->setColumnStretch(1, 1);
    leftLayout->addWidget(metricRow);

    m_deltaBar = createThinProgress(QStringLiteral("#a855f7"), leftPanel);
    leftLayout->addWidget(m_deltaBar);

    QWidget *modeRow = new QWidget(leftPanel);
    QHBoxLayout *modeLayout = new QHBoxLayout(modeRow);
    modeLayout->setContentsMargins(0, 0, 0, 0);
    modeLayout->setSpacing(16);

    QLabel *modeLabel = new QLabel(QString::fromUtf8("测量模式"), modeRow);
    modeLabel->setObjectName("offsetModeLabel");

    m_singleRadio = new QRadioButton(QString::fromUtf8("单次测量"), modeRow);
    m_singleRadio->setObjectName("offsetModeRadioButton");
    m_continuousRadio = new QRadioButton(QString::fromUtf8("连续测量"), modeRow);
    m_continuousRadio->setObjectName("offsetModeRadioButton");
    m_singleRadio->setChecked(true);

    connect(m_singleRadio, &QRadioButton::clicked, this, &OffsetMeasurementPage::updateModeSelection);
    connect(m_continuousRadio, &QRadioButton::clicked, this, &OffsetMeasurementPage::updateModeSelection);

    modeLayout->addWidget(modeLabel);
    modeLayout->addWidget(m_singleRadio);
    modeLayout->addWidget(m_continuousRadio);
    modeLayout->addStretch();
    leftLayout->addWidget(modeRow);

    QWidget *buttonRow = new QWidget(leftPanel);
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonRow);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(14);

    m_startButton = createActionButton(QString::fromUtf8("开始测量"), true, buttonRow);
    m_repeatButton = createActionButton(QString::fromUtf8("重新测量"), false, buttonRow);
    connect(m_startButton, &QPushButton::clicked, this, &OffsetMeasurementPage::startOrStopMeasurement);
    connect(m_repeatButton, &QPushButton::clicked, this, &OffsetMeasurementPage::repeatMeasurement);

    buttonLayout->addWidget(m_startButton);
    buttonLayout->addWidget(m_repeatButton);
    buttonLayout->addStretch();
    leftLayout->addWidget(buttonRow);
    leftLayout->addStretch();

    QGroupBox *rightPanel = new QGroupBox(QString(), topSection);
    rightPanel->setObjectName("sectionBox");
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(4, 4, 4, 4);
    rightLayout->setSpacing(4);

    m_channelTable = new QTableWidget(5, 5, rightPanel);
    m_channelTable->setObjectName("pageTable");
    m_channelTable->setHorizontalHeaderLabels(QStringList()
                                              << QString::fromUtf8("通道")
                                              << QString::fromUtf8("输出内容")
                                              << QString::fromUtf8("相位补偿(ns)")
                                              << QString::fromUtf8("1PPS偏差")
                                              << QString());
    m_channelTable->verticalHeader()->setVisible(false);
    m_channelTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_channelTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_channelTable->setFocusPolicy(Qt::NoFocus);
    m_channelTable->setShowGrid(true);
    m_channelTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_channelTable->setMinimumHeight(0);

    QFont tableFont = m_channelTable->font();
    tableFont.setPointSize(11);
    m_channelTable->setFont(tableFont);

    m_channelTable->horizontalHeader()->setFixedHeight(52);
    m_channelTable->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    m_channelTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_channelTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_channelTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_channelTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_channelTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    m_channelTable->setColumnWidth(4, 190);

    QHeaderView *header = m_channelTable->horizontalHeader();
    header->setStretchLastSection(false);
    header->setSectionsClickable(false);
    header->viewport()->setStyleSheet(QStringLiteral("background: transparent;"));

    m_enableHeaderWidget = new QWidget(header->viewport());
    m_enableHeaderWidget->setAttribute(Qt::WA_StyledBackground, false);

    QHBoxLayout *enableHeaderLayout = new QHBoxLayout(m_enableHeaderWidget);
    enableHeaderLayout->setContentsMargins(0, 0, 0, 0);
    enableHeaderLayout->setSpacing(14);
    enableHeaderLayout->setAlignment(Qt::AlignCenter);

    m_enableHeaderLabel = new QLabel(QString::fromUtf8("启用"), m_enableHeaderWidget);
    QFont enableHeaderFont = m_channelTable->font();
    enableHeaderFont.setPointSize(11);
    enableHeaderFont.setBold(true);
    m_enableHeaderLabel->setFont(enableHeaderFont);
    m_enableHeaderLabel->setStyleSheet(QStringLiteral("color:#b9d5ff;background: transparent;"));

    m_selectAllButton = new QPushButton(QString::fromUtf8("全选"), m_enableHeaderWidget);
    m_selectAllButton->setFixedSize(44, 22);
    QFont selectButtonFont = m_selectAllButton->font();
    selectButtonFont.setPointSize(8);
    selectButtonFont.setBold(false);
    m_selectAllButton->setFont(selectButtonFont);
    m_selectAllButton->setCursor(Qt::PointingHandCursor);
    m_selectAllButton->setFocusPolicy(Qt::NoFocus);
    m_selectAllButton->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "color:#dbeafe;"
        "background: rgba(23, 45, 86, 0.95);"
        "border: 1px solid #35548b;"
        "border-radius: 3px;"
        "padding: 0px 6px;"
        "}"
        "QPushButton:hover {"
        "background: rgba(36, 63, 112, 0.98);"
        "border-color:#4b70b4;"
        "}"
    ));
    enableHeaderLayout->addWidget(m_enableHeaderLabel, 0, Qt::AlignVCenter);
    enableHeaderLayout->addWidget(m_selectAllButton, 0, Qt::AlignVCenter);

    connect(m_selectAllButton, &QPushButton::clicked, this, &OffsetMeasurementPage::selectAllChannels);
    connect(header, &QHeaderView::sectionResized, this, [this](int logicalIndex, int, int) {
        if (logicalIndex == 4) {
            repositionEnableHeaderControls();
        }
    });
    connect(header, &QHeaderView::geometriesChanged, this, [this]() {
        repositionEnableHeaderControls();
    });

    m_channelTable->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_channelTable->verticalHeader()->setDefaultSectionSize(46);
    rightLayout->addWidget(m_channelTable, 1);

    topLayout->addWidget(leftPanel, 5);
    topLayout->addWidget(rightPanel, 8);
    layout->addWidget(topSection, 1);

    QFrame *chartFrame = new QFrame(card);
    chartFrame->setObjectName("chartFrame");
    QVBoxLayout *chartLayout = new QVBoxLayout(chartFrame);
    chartLayout->setContentsMargins(14, 10, 14, 14);
    chartLayout->setSpacing(6);

    m_trendChart = new TrendChartWidget(chartFrame);
    chartLayout->addWidget(m_trendChart, 1);

    layout->addWidget(chartFrame, 2);

    m_measureTimer->setInterval(1000);
    connect(m_measureTimer, &QTimer::timeout, this, &OffsetMeasurementPage::updateMeasurement);

    refreshChannelTable();
    refreshUi();
    repositionEnableHeaderControls();
}

void OffsetMeasurementPage::updateModeSelection()
{
    if (m_measuring) {
        return;
    }

    m_continuousMode = m_continuousRadio->isChecked();
    refreshUi();
}

void OffsetMeasurementPage::startOrStopMeasurement()
{
    if (!m_measuring) {
        if (!hasEnabledChannels()) {
            emit logMessage(QString::fromUtf8("时差测量：请先至少启用一个测量通道"));
            return;
        }

        m_continuousMode = m_continuousRadio->isChecked();
        m_measuring = true;
        m_measureTimer->start();

        emit logMessage(m_continuousMode
                        ? QString::fromUtf8("时差测量：连续测量已启动")
                        : QString::fromUtf8("时差测量：单次测量已启动"));
    } else {
        m_measureTimer->stop();
        m_measuring = false;
        emit logMessage(QString::fromUtf8("时差测量：测量已停止"));
    }

    refreshChannelTable();
    refreshUi();
}

void OffsetMeasurementPage::resetHistory()
{
    m_historySamples.clear();
    m_lastSamples.fill(0.0);
    m_channelHistorySamples = QVector<QVector<double> >(5);
    m_trendChart->setChannelSamples(m_channelHistorySamples, m_channelEnabled);
}

void OffsetMeasurementPage::repeatMeasurement()
{
    m_measureTimer->stop();
    m_measuring = false;
    m_elapsedSeconds = 0;
    resetHistory();

    emit measurementChanged(QString::fromUtf8("0.0 ns"));
    emit logMessage(QString::fromUtf8("时差测量：已重新测量并清空历史数据"));

    refreshChannelTable();
    refreshUi();
}

void OffsetMeasurementPage::updateMeasurement()
{
    QVector<double> samples(5, 0.0);

    for (int i = 0; i < 5; ++i) {
        if (!m_channelEnabled.value(i, false)) {
            continue;
        }

        const double base = (i - 2) * 6.5;
        const double jitter = QRandomGenerator::global()->bounded(41) - 20.0;
        const double drift = qSin((m_elapsedSeconds / 10.0) + i * 0.6) * (8.0 + i * 1.5);
        samples[i] = base + drift + jitter;
    }

    double average = 0.0;
    int enabledCount = 0;
    for (int i = 0; i < samples.size(); ++i) {
        if (m_channelEnabled.value(i, false)) {
            average += samples.at(i);
            ++enabledCount;
        }
    }
    if (enabledCount > 0) {
        average /= enabledCount;
    }

    double sigma = 0.0;
    for (int i = 0; i < samples.size(); ++i) {
        if (m_channelEnabled.value(i, false)) {
            sigma += qPow(samples.at(i) - average, 2.0);
        }
    }
    sigma = enabledCount > 0 ? qSqrt(sigma / enabledCount) : 0.0;

    m_elapsedSeconds += 8;

    m_lastSamples = samples;
    m_historySamples.append(average);
    if (m_historySamples.size() > 180) {
        m_historySamples.remove(0, m_historySamples.size() - 180);
    }

    for (int i = 0; i < samples.size(); ++i) {
        if (!m_channelEnabled.value(i, false)) {
            continue;
        }
        m_channelHistorySamples[i].append(samples.at(i));
        if (m_channelHistorySamples[i].size() > 180) {
            m_channelHistorySamples[i].remove(0, m_channelHistorySamples[i].size() - 180);
        }
    }

    m_deltaValue->setText(QString::fromUtf8("%1 ns").arg(QString::number(average, 'f', 1)));
    m_sigmaValue->setText(QString::fromUtf8("%1 ppb").arg(QString::number(qMax(0.5, sigma / 10.0), 'f', 2)));
    m_durationValue->setText(QString::fromUtf8("%1 s").arg(m_elapsedSeconds));
    m_deltaBar->setValue(qMin(100, qRound(qAbs(average) * 2.0)));
    m_trendChart->setChannelSamples(m_channelHistorySamples, m_channelEnabled);

    refreshChannelTable();

    QString summary = QString::fromUtf8("%1 ns").arg(QString::number(average, 'f', 1));
    if (qAbs(average) > 45.0) {
        summary = QString::fromUtf8("告警 %1").arg(summary);
    }
    emit measurementChanged(summary);

    if (!m_continuousMode) {
        m_measureTimer->stop();
        m_measuring = false;
        emit logMessage(QString::fromUtf8("时差测量：已完成一次单次测量"));
    }

    refreshUi();
}

void OffsetMeasurementPage::refreshUi()
{
    m_modeValue->setText(m_continuousRadio && m_continuousRadio->isChecked()
                         ? QString::fromUtf8("连续测量")
                         : QString::fromUtf8("单次测量"));

    if (m_measuring) {
        m_stateValue->setText(QString::fromUtf8("测量中"));
        m_startButton->setText(QString::fromUtf8("停止测量"));
    } else {
        m_stateValue->setText(m_elapsedSeconds > 0 ? QString::fromUtf8("已停止") : QString::fromUtf8("未开始"));
        m_startButton->setText(QString::fromUtf8("开始测量"));
    }

    const bool lockSelection = m_measuring;
    m_singleRadio->setEnabled(!lockSelection);
    m_continuousRadio->setEnabled(!lockSelection);
    m_selectAllButton->setEnabled(!lockSelection);
    m_repeatButton->setEnabled(!m_measuring);

    refreshChannelTable();
    updateSelectAllState();
}

void OffsetMeasurementPage::handleChannelCheckChanged(bool checked)
{
    QCheckBox *checkBox = qobject_cast<QCheckBox *>(sender());
    if (!checkBox) {
        return;
    }

    bool ok = false;
    const int row = checkBox->property("row").toInt(&ok);
    if (!ok || row < 0 || row >= m_channelEnabled.size()) {
        return;
    }

    if (m_measuring) {
        QSignalBlocker blocker(checkBox);
        checkBox->setChecked(m_channelEnabled[row]);
        return;
    }

    m_channelEnabled[row] = checked;
    if (!m_channelEnabled[row]) {
        m_channelHistorySamples[row].clear();
    }
    m_trendChart->setChannelSamples(m_channelHistorySamples, m_channelEnabled);

    emit logMessage(QString::fromUtf8("时差测量：CH%1 %2")
                    .arg(row + 1)
                    .arg(m_channelEnabled[row] ? QString::fromUtf8("已启用")
                                               : QString::fromUtf8("已禁用")));

    updateSelectAllState();
}

void OffsetMeasurementPage::refreshChannelTable()
{
    if (!m_channelTable) {
        return;
    }

    m_channelTable->setRowCount(5);

    for (int row = 0; row < 5; ++row) {
        const bool enabled = m_channelEnabled.value(row, false);
        const double current = m_lastSamples.value(row, 0.0);
        const QVector<double> history = m_channelHistorySamples.value(row);

        double average = 0.0;
        for (double value : history) {
            average += value;
        }
        average = history.isEmpty() ? 0.0 : average / history.size();

        setCenterItem(m_channelTable, row, 0, QString::fromUtf8("CH%1").arg(row + 1));
        setCenterItem(m_channelTable, row, 1, QString::fromUtf8(""));
        setCenterItem(m_channelTable, row, 2, enabled ? QString::number(average, 'f', 1) : QString::fromUtf8(""));
        setCenterItem(m_channelTable, row, 3, enabled ? QString::number(current, 'f', 1) : QString::fromUtf8(""));

        QTableWidgetItem *channelItem = m_channelTable->item(row, 0);
        if (channelItem) {
            channelItem->setForeground(enabled ? channelColor(row) : QColor("#e5e7eb"));
        }

        QWidget *cellWidget = m_channelTable->cellWidget(row, 4);
        ClickableCheckCellWidget *checkHost = static_cast<ClickableCheckCellWidget *>(cellWidget);
        if (!checkHost) {
            checkHost = new ClickableCheckCellWidget(m_channelTable);
            checkHost->checkBox()->setProperty("row", row);
            m_channelTable->setCellWidget(row, 4, checkHost);
            connect(checkHost->checkBox(), &QCheckBox::toggled, this, &OffsetMeasurementPage::handleChannelCheckChanged);
        }

        {
            QSignalBlocker blocker(checkHost->checkBox());
            checkHost->setChecked(enabled);
            checkHost->checkBox()->setEnabled(!m_measuring);
            checkHost->setEnabled(true);
        }
    }

    repositionEnableHeaderControls();
    updateSelectAllState();
}

void OffsetMeasurementPage::repositionEnableHeaderControls()
{
    if (!m_channelTable || !m_enableHeaderWidget || !m_selectAllButton || !m_enableHeaderLabel) {
        return;
    }

    QHeaderView *header = m_channelTable->horizontalHeader();
    if (!header) {
        return;
    }

    const int sectionX = header->sectionPosition(4);
    const int sectionWidth = header->sectionSize(4);
    const int sectionHeight = header->height();
    const int contentWidth = m_enableHeaderLabel->sizeHint().width() + 14 + m_selectAllButton->width();
    const int contentHeight = qMax(m_enableHeaderLabel->sizeHint().height(), m_selectAllButton->height());
    const int contentX = sectionX + qMax(0, (sectionWidth - contentWidth) / 2);
    const int contentY = qMax(0, (sectionHeight - contentHeight) / 2);

    m_enableHeaderWidget->setGeometry(contentX, contentY, contentWidth, contentHeight);
    m_enableHeaderWidget->raise();
    m_enableHeaderWidget->show();
}

bool OffsetMeasurementPage::areAllChannelsChecked() const
{
    return enabledChannelCount(m_channelEnabled) == m_channelEnabled.size() && !m_channelEnabled.isEmpty();
}

void OffsetMeasurementPage::setAllChannelsChecked(bool checked)
{
    for (int i = 0; i < m_channelEnabled.size(); ++i) {
        m_channelEnabled[i] = checked;
        if (!checked) {
            m_channelHistorySamples[i].clear();
        }
    }

    m_trendChart->setChannelSamples(m_channelHistorySamples, m_channelEnabled);
    refreshChannelTable();
}

void OffsetMeasurementPage::updateSelectAllState()
{
    if (m_selectAllButton) {
        const bool allChecked = areAllChannelsChecked();
        m_selectAllButton->setText(QString::fromUtf8("全选"));
        m_selectAllButton->setToolTip(allChecked
                                      ? QString::fromUtf8("取消全选")
                                      : QString::fromUtf8("全选通道"));
    }
}

void OffsetMeasurementPage::selectAllChannels()
{
    if (m_measuring) {
        return;
    }

    const bool target = !areAllChannelsChecked();
    setAllChannelsChecked(target);

    emit logMessage(QString::fromUtf8("时差测量：通道已%1")
                    .arg(target ? QString::fromUtf8("全选")
                                : QString::fromUtf8("全不选")));
}

bool OffsetMeasurementPage::hasEnabledChannels() const
{
    return enabledChannelCount(m_channelEnabled) > 0;
}
