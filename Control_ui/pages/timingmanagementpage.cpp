#include "timingmanagementpage.h"

#include "detailuifactory.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QFrame>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPlainTextEdit>
#include <QResizeEvent>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSize>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

using namespace DetailUiFactory;

namespace {

QString templateForFormat(const QString &formatText)
{
    if (formatText.contains(QString::fromUtf8("标准"))) {
        //return QString::fromUtf8("$TOD,hh:mm:ss,yyyy-MM-dd,zone=*CS\n$MODE,ASCII_STD,OUTPUT=ENABLED\n$SYNC,INTERVAL=1S\n");
    }
    if (formatText.contains(QString::fromUtf8("扩展"))) {
        //return QString::fromUtf8("$TOD,week,seconds,yyyyMMdd,zone,quality=*CS\n$MODE,ASCII_EXT,OUTPUT=ENABLED\n$SYNC,INTERVAL=1S,HEAD=CUSTOM\n");
    }
    //return QString::fromUtf8("$TOD,hhmmss,yyyyMMdd,zone=*CS\n$MODE,1PPS,OUTPUT=5\n");
}

void setCenterItem(QTableWidget *table, int row, int col, const QString &text)
{
    QTableWidgetItem *item = table->item(row, col);
    if (!item) {
        item = new QTableWidgetItem;
        item->setTextAlignment(Qt::AlignCenter);
        table->setItem(row, col, item);
    }
    item->setText(text);
}

QWidget *createControlRow(QWidget *parent,
                          const QString &titleText,
                          QLabel **descriptionLabel,
                          QPushButton *button)
{
    QWidget *row = new QWidget(parent);
    QHBoxLayout *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(12);

    QWidget *textHost = new QWidget(row);
    QVBoxLayout *textLayout = new QVBoxLayout(textHost);
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(3);

    QLabel *titleLabel = new QLabel(titleText, textHost);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(13);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet(QStringLiteral("color:#eaf2ff;"));

    QLabel *descLabel = new QLabel(QString::fromUtf8("--"), textHost);
    QFont descFont = descLabel->font();
    descFont.setPointSize(10);
    descLabel->setFont(descFont);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet(QStringLiteral("color:#9fb4d9;"));

    textLayout->addWidget(titleLabel);
    textLayout->addWidget(descLabel);

    if (button) {
        button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }

    rowLayout->addWidget(textHost, 1);
    rowLayout->addWidget(button, 0, Qt::AlignRight | Qt::AlignVCenter);

    if (descriptionLabel) {
        *descriptionLabel = descLabel;
    }
    return row;
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
        if (event->button() == Qt::LeftButton) {
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

TimingManagementPage::TimingManagementPage(QWidget *parent)
    : QWidget(parent)
    , m_sourceText(QString())
    , m_ntpEnabled(false)
    , m_outputsEnabled(false)
    , m_synced(false)
    , m_currentTimeText(QString())
    , m_lastSyncText(QString())
    , m_sourceDescriptionLabel(nullptr)
    , m_syncDescriptionLabel(nullptr)
    , m_ntpDescriptionLabel(nullptr)
    , m_outputTable(nullptr)
    , m_todFormatCombo(nullptr)
    , m_todEditor(nullptr)
    , m_sourceButton(nullptr)
    , m_syncButton(nullptr)
    , m_ntpButton(nullptr)
    , m_applyButton(nullptr)
    , m_selectAllButton(nullptr)
    , m_enableHeaderWidget(nullptr)
    , m_enableHeaderLabel(nullptr)
{
    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    QFrame *card = createCardFrame(this);
    rootLayout->addWidget(card);

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(22, 20, 22, 18);
    layout->setSpacing(10);


    QWidget *topRow = new QWidget(card);
    topRow->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    QHBoxLayout *topLayout = new QHBoxLayout(topRow);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(8);

    QGroupBox *overviewBox = new QGroupBox(QString::fromUtf8("授时控制"), topRow);
    overviewBox->setObjectName("sectionBox");
    QVBoxLayout *overviewLayout = new QVBoxLayout(overviewBox);
    overviewLayout->setContentsMargins(16, 12, 12, 12);
    overviewLayout->setSpacing(8);

    m_sourceButton = createActionButton(QString(), false, overviewBox);
    m_syncButton = createActionButton(QString(), false, overviewBox);
    m_ntpButton = createActionButton(QString(), false, overviewBox);

    initializeIconButton(m_sourceButton, QString::fromUtf8("切换输出模式"));
    initializeIconButton(m_syncButton, QString::fromUtf8("立即同步"));
    initializeIconButton(m_ntpButton, QString::fromUtf8("开启或关闭 NTP"));

    connect(m_sourceButton, &QPushButton::clicked, this, &TimingManagementPage::switchSource);
    connect(m_syncButton, &QPushButton::clicked, this, &TimingManagementPage::syncNow);
    connect(m_ntpButton, &QPushButton::clicked, this, &TimingManagementPage::toggleNtp);

    overviewLayout->addWidget(createControlRow(overviewBox,
                                               QString::fromUtf8("输出模式"),
                                               &m_sourceDescriptionLabel,
                                               m_sourceButton));
    overviewLayout->addStretch(1);
    overviewLayout->addWidget(createControlRow(overviewBox,
                                               QString::fromUtf8("同步控制"),
                                               &m_syncDescriptionLabel,
                                               m_syncButton));
    overviewLayout->addStretch(1);
    overviewLayout->addWidget(createControlRow(overviewBox,
                                               QString::fromUtf8("网络授时"),
                                               &m_ntpDescriptionLabel,
                                               m_ntpButton));

    QGroupBox *channelBox = new QGroupBox(QString(), topRow);
    channelBox->setObjectName("sectionBox");
    QVBoxLayout *channelLayout = new QVBoxLayout(channelBox);
    channelLayout->setContentsMargins(4, 4, 4, 4);
    channelLayout->setSpacing(2);

    m_outputTable = new QTableWidget(5, 5, channelBox);
    m_outputTable->setObjectName("pageTable");
    m_outputTable->setHorizontalHeaderLabels(QStringList()
                                             << QString::fromUtf8("通道")
                                             << QString::fromUtf8("输出内容")
                                             << QString::fromUtf8("相位补偿(ns)")
                                             << QString::fromUtf8("1PPS偏差")
                                             << QString());
    m_outputTable->verticalHeader()->setVisible(false);
    m_outputTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_outputTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_outputTable->setFocusPolicy(Qt::NoFocus);
    m_outputTable->setShowGrid(true);
    m_outputTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_outputTable->setMinimumHeight(0);

    QFont tableFont = m_outputTable->font();
    tableFont.setPointSize(11);
    m_outputTable->setFont(tableFont);

    m_outputTable->horizontalHeader()->setFixedHeight(52);
    m_outputTable->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    m_outputTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_outputTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_outputTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_outputTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_outputTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    m_outputTable->setColumnWidth(4, 190);

    QHeaderView *header = m_outputTable->horizontalHeader();
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
    QFont enableHeaderFont = m_outputTable->font();
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

    connect(m_selectAllButton, &QPushButton::clicked, this, &TimingManagementPage::handleSelectAllButtonClicked);
    connect(header, &QHeaderView::sectionResized, this, [this](int logicalIndex, int, int) {
        if (logicalIndex == 4) {
            repositionEnableHeaderControls();
        }
    });
    connect(header, &QHeaderView::geometriesChanged, this, [this]() {
        repositionEnableHeaderControls();
    });

    m_outputTable->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_outputTable->verticalHeader()->setDefaultSectionSize(46);
    channelLayout->addWidget(m_outputTable, 1);

    repositionEnableHeaderControls();

    topLayout->addWidget(overviewBox, 4);
    topLayout->addWidget(channelBox, 10);

    QGroupBox *todBox = new QGroupBox(QString::fromUtf8("ToD 格式设置"), card);
    todBox->setObjectName("sectionBox");
    todBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout *todLayout = new QVBoxLayout(todBox);
    todLayout->setContentsMargins(16, 14, 16, 16);
    todLayout->setSpacing(10);

    QWidget *formatRow = new QWidget(todBox);
    QHBoxLayout *formatLayout = new QHBoxLayout(formatRow);
    formatLayout->setContentsMargins(0, 0, 0, 0);
    formatLayout->setSpacing(10);

    QLabel *formatLabel = new QLabel(QString::fromUtf8("输出格式"), formatRow);
    QFont formatLabelFont = formatLabel->font();
    formatLabelFont.setPointSize(12);
    formatLabel->setFont(formatLabelFont);

    m_todFormatCombo = new QComboBox(formatRow);
    m_todFormatCombo->setEditable(true);
    m_todFormatCombo->addItems(QStringList()
                               << QString::fromUtf8("ASCII-紧凑格式")
                               << QString::fromUtf8("ASCII-标准格式")
                               << QString::fromUtf8("ASCII-扩展格式")
                               << QString::fromUtf8("自定义格式"));
    m_todFormatCombo->setMinimumHeight(38);
    QFont comboFont = m_todFormatCombo->font();
    comboFont.setPointSize(12);
    m_todFormatCombo->setFont(comboFont);
    connect(m_todFormatCombo, &QComboBox::currentTextChanged, this, &TimingManagementPage::handleFormatChanged);

    formatLayout->addWidget(formatLabel);
    formatLayout->addWidget(m_todFormatCombo, 1);
    todLayout->addWidget(formatRow);

    m_todEditor = new QPlainTextEdit(todBox);
    m_todEditor->setObjectName("configEditor");
    m_todEditor->setMinimumHeight(410);
    QFont editorFont(QStringLiteral("Consolas"));
    editorFont.setPointSize(15);
    m_todEditor->setFont(editorFont);
    todLayout->addWidget(m_todEditor, 1);

    m_applyButton = createActionButton(QString::fromUtf8("应用输出设置"), true, todBox);
    m_applyButton->setMinimumHeight(54);
    m_applyButton->setMinimumWidth(168);
    QFont applyFont = m_applyButton->font();
    applyFont.setPointSize(12);
    m_applyButton->setFont(applyFont);
    connect(m_applyButton, &QPushButton::clicked, this, &TimingManagementPage::applyOutputConfig);
    todLayout->addWidget(m_applyButton, 0, Qt::AlignLeft);

    layout->addWidget(topRow, 1);
    layout->addWidget(todBox, 2);
    layout->setStretchFactor(topRow, 1);
    layout->setStretchFactor(todBox, 2);

    initializeChannelTable();
    clearTimingStatus();
}

void TimingManagementPage::initializeIconButton(QPushButton *button, const QString &toolTipText)
{
    if (!button) {
        return;
    }

    button->setText(QString());
    button->setFixedSize(84, 84);
    button->setIconSize(QSize(52, 52));
    button->setCursor(Qt::PointingHandCursor);
    button->setToolTip(toolTipText);
    button->setCheckable(false);
    button->setFlat(false);
    button->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "padding: 0px;"
        "margin: 0px;"
        "border-radius: 14px;"
        "}"
    ));
}

QString TimingManagementPage::selectedFormatText() const
{
    return m_todFormatCombo ? m_todFormatCombo->currentText() : QString::fromUtf8("未选择");
}

void TimingManagementPage::repositionEnableHeaderControls()
{
    if (!m_outputTable || !m_enableHeaderWidget || !m_selectAllButton || !m_enableHeaderLabel) {
        return;
    }

    QHeaderView *header = m_outputTable->horizontalHeader();
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
}

void TimingManagementPage::setTimingStatus(const QString &sourceText,
                                           bool ntpEnabled,
                                           bool outputsEnabled,
                                           bool synced,
                                           const QString &currentTimeText,
                                           const QString &lastSyncText)
{
    m_sourceText = sourceText;
    m_ntpEnabled = ntpEnabled;
    m_outputsEnabled = outputsEnabled;
    m_synced = synced;
    m_currentTimeText = currentTimeText;
    m_lastSyncText = lastSyncText;
    refreshUi();
}

void TimingManagementPage::clearTimingStatus()
{
    m_sourceText = QString::fromUtf8("等待数据");
    m_ntpEnabled = false;
    m_outputsEnabled = false;
    m_synced = false;
    m_currentTimeText = QString::fromUtf8("--");
    m_lastSyncText = QString::fromUtf8("--");
    if (m_todFormatCombo) {
        m_todFormatCombo->setCurrentText(QString::fromUtf8("ASCII-紧凑格式"));
    }
    handleFormatChanged(selectedFormatText());
    refreshUi();
}

void TimingManagementPage::initializeChannelTable()
{
    for (int row = 0; row < m_outputTable->rowCount(); ++row) {
        setChannelRowData(row, QString(), QString(), QString(), QString());
    }
    updateSelectAllState();
}

void TimingManagementPage::clearChannelContents()
{
    for (int row = 0; row < m_outputTable->rowCount(); ++row) {
        setChannelRowData(row, QString(), QString(), QString(), QString());
    }
    updateSelectAllState();
}

void TimingManagementPage::setChannelRowData(int row,
                                             const QString &content,
                                             const QString &phaseOffsetNs,
                                             const QString &jitterText,
                                             const QString &stateText)
{
    if (row < 0 || row >= m_outputTable->rowCount()) {
        return;
    }

    setCenterItem(m_outputTable, row, 0, QString::fromUtf8("CH%1").arg(row + 1));
    setCenterItem(m_outputTable, row, 1, content.trimmed());
    setCenterItem(m_outputTable, row, 2, phaseOffsetNs.trimmed());
    setCenterItem(m_outputTable, row, 3, jitterText.trimmed());

    QWidget *cellWidget = m_outputTable->cellWidget(row, 4);
    ClickableCheckCellWidget *checkHost = static_cast<ClickableCheckCellWidget *>(cellWidget);
    if (!checkHost) {
        checkHost = new ClickableCheckCellWidget(m_outputTable);
        checkHost->checkBox()->setProperty("row", row);
        m_outputTable->setCellWidget(row, 4, checkHost);
        connect(checkHost->checkBox(), &QCheckBox::toggled, this, &TimingManagementPage::handleChannelCheckToggled);
    }

    const bool enabled = stateText.contains(QString::fromUtf8("已应用"))
                      || stateText.contains(QString::fromUtf8("已输出"))
                      || stateText.contains(QString::fromUtf8("开启"));
    QSignalBlocker blocker(checkHost->checkBox());
    checkHost->setChecked(enabled);

    updateSelectAllState();
}

void TimingManagementPage::refreshButtonIcons()
{
    if (m_sourceButton) {
        m_sourceButton->setText(QString());
        m_sourceButton->setIcon(QIcon(QStringLiteral(":/image/resources/qiehuan.png")));
        m_sourceButton->setIconSize(QSize(52, 52));
        m_sourceButton->setToolTip(QString::fromUtf8("切换输出模式"));
    }

    if (m_syncButton) {
        m_syncButton->setText(QString());
        m_syncButton->setIcon(QIcon(QStringLiteral(":/image/resources/tongbu.png")));
        m_syncButton->setIconSize(QSize(52, 52));
        m_syncButton->setToolTip(QString::fromUtf8("立即同步"));
    }

    if (m_ntpButton) {
        m_ntpButton->setText(QString());
        m_ntpButton->setIcon(QIcon(m_ntpEnabled
                                   ? QStringLiteral(":/image/resources/on.png")
                                   : QStringLiteral(":/image/resources/off.png")));
        m_ntpButton->setIconSize(QSize(52, 52));
        m_ntpButton->setToolTip(m_ntpEnabled
                                ? QString::fromUtf8("关闭 NTP")
                                : QString::fromUtf8("开启 NTP"));
    }
}

void TimingManagementPage::refreshUi()
{
    refreshButtonIcons();

    if (m_sourceDescriptionLabel) {
        m_sourceDescriptionLabel->setText(QString::fromUtf8("当前模式：%1").arg(m_sourceText));
    }
    if (m_syncDescriptionLabel) {
        m_syncDescriptionLabel->setText(QString::fromUtf8("同步状态：%1  ·  最后同步：%2")
                                        .arg(m_synced ? QString::fromUtf8("已同步") : QString::fromUtf8("待同步"),
                                             m_lastSyncText));
    }
    if (m_ntpDescriptionLabel) {
        m_ntpDescriptionLabel->setText(QString::fromUtf8("NTP：%1  ·  当前授时时间：%2")
                                       .arg(m_ntpEnabled ? QString::fromUtf8("开启") : QString::fromUtf8("关闭"),
                                            m_currentTimeText));
    }

    emit clockChanged(QString::fromUtf8("%1 · %2 · %3")
                      .arg(m_sourceText,
                           m_ntpEnabled ? QString::fromUtf8("NTP开") : QString::fromUtf8("NTP关"),
                           m_synced ? QString::fromUtf8("已同步") : QString::fromUtf8("待同步")));
}

void TimingManagementPage::syncNow()
{
    emit syncRequested();
    m_synced = true;
    m_currentTimeText = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));
    m_lastSyncText = QString::fromUtf8("刚刚");
    refreshUi();
    emit logMessage(QString::fromUtf8("授时管理：已执行立即同步"));
}

void TimingManagementPage::toggleNtp()
{
    m_ntpEnabled = !m_ntpEnabled;
    emit ntpToggleRequested(m_ntpEnabled);
    refreshUi();
    emit logMessage(QString::fromUtf8("授时管理：NTP 已%1").arg(m_ntpEnabled ? QString::fromUtf8("开启") : QString::fromUtf8("关闭")));
}

void TimingManagementPage::switchSource()
{
    m_sourceText = (m_sourceText == QString::fromUtf8("1PPS + ToD")) ? QString::fromUtf8("GNSS") : QString::fromUtf8("1PPS + ToD");
    emit sourceSwitchRequested(m_sourceText);
    refreshUi();
    emit logMessage(QString::fromUtf8("授时管理：输出模式已切换为 %1").arg(m_sourceText));
}

void TimingManagementPage::applyOutputConfig()
{
    bool hasAnyEnabled = false;
    for (int row = 0; row < m_outputTable->rowCount(); ++row) {
        ClickableCheckCellWidget *checkHost =
            static_cast<ClickableCheckCellWidget *>(m_outputTable->cellWidget(row, 4));
        const bool enabled = checkHost && checkHost->isChecked();
        hasAnyEnabled = hasAnyEnabled || enabled;
    }

    m_outputsEnabled = hasAnyEnabled;
    emit outputsToggleRequested(m_outputsEnabled);
    emit timingOutputApplied(QString::fromUtf8("TIMING"),
                             QString::fromUtf8("授时输出"),
                             QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")),
                             QString::fromUtf8("已应用"));
    emit logMessage(QString::fromUtf8("授时管理：输出设置已应用，当前格式：%1").arg(selectedFormatText()));
    QMessageBox::information(this, QString::fromUtf8("授时管理"), QString::fromUtf8("输出设置已应用。"));
}

void TimingManagementPage::handleFormatChanged(const QString &formatText)
{
    if (!m_todEditor) {
        return;
    }

    const QString currentText = m_todEditor->toPlainText().trimmed();
    if (currentText.isEmpty() || currentText.startsWith(QStringLiteral("$TOD,"))) {
        m_todEditor->setPlainText(templateForFormat(formatText));
    }
}

bool TimingManagementPage::areAllChannelsChecked() const
{
    if (!m_outputTable || m_outputTable->rowCount() <= 0) {
        return false;
    }

    for (int row = 0; row < m_outputTable->rowCount(); ++row) {
        ClickableCheckCellWidget *checkHost =
            static_cast<ClickableCheckCellWidget *>(m_outputTable->cellWidget(row, 4));
        if (!checkHost || !checkHost->isChecked()) {
            return false;
        }
    }
    return true;
}

void TimingManagementPage::setAllChannelsChecked(bool checked)
{
    for (int row = 0; row < m_outputTable->rowCount(); ++row) {
        ClickableCheckCellWidget *checkHost =
            static_cast<ClickableCheckCellWidget *>(m_outputTable->cellWidget(row, 4));
        if (!checkHost) {
            continue;
        }
        checkHost->setChecked(checked);
    }

    m_outputsEnabled = checked;
    emit outputsToggleRequested(m_outputsEnabled);
}

void TimingManagementPage::handleSelectAllButtonClicked()
{
    const bool targetChecked = !areAllChannelsChecked();
    setAllChannelsChecked(targetChecked);
    updateSelectAllState();
}

void TimingManagementPage::handleChannelCheckToggled(bool)
{
    updateSelectAllState();
    emit outputsToggleRequested(m_outputsEnabled);
}

void TimingManagementPage::updateSelectAllState()
{
    bool anyChecked = false;
    bool allChecked = true;
    for (int row = 0; row < m_outputTable->rowCount(); ++row) {
        ClickableCheckCellWidget *checkHost =
            static_cast<ClickableCheckCellWidget *>(m_outputTable->cellWidget(row, 4));
        const bool checked = checkHost && checkHost->isChecked();
        anyChecked = anyChecked || checked;
        allChecked = allChecked && checked;
    }

    if (m_selectAllButton) {
        repositionEnableHeaderControls();
        m_selectAllButton->setText(QString::fromUtf8("全选"));
        m_selectAllButton->setToolTip(allChecked && m_outputTable->rowCount() > 0
                                      ? QString::fromUtf8("取消全选")
                                      : QString::fromUtf8("全选通道"));
    }
    m_outputsEnabled = anyChecked;
}
