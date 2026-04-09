#include "mainwindow.h"

#include "datamanagementpage.h"
#include "flowcontrolpage.h"
#include "offsetmeasurementpage.h"
#include "powermanagementpage.h"
#include "previewcardwidget.h"
#include "settingspage.h"
#include "subclockmeasurementpage.h"
#include "timingmanagementpage.h"
#include "detailuifactory.h"

#include <QApplication>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <QStyle>
using namespace DetailUiFactory;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_stackedWidget(nullptr)
    , m_detailTitleLabel(nullptr)
    , m_detailSubtitleLabel(nullptr)
    , m_footerStateLabel(nullptr)
    , m_footerClockLabel(nullptr)
    , m_footerWriteRateLabel(nullptr)
    , m_dataButton(nullptr)
    , m_settingsButton(nullptr)
    , m_titleBarWidget(nullptr)
    , m_flowToolbar(nullptr)
    , m_flowToolbarPauseButton(nullptr)
    , m_draggingWindow(false)
    , m_powerManagementPage(nullptr)
    , m_timingManagementPage(nullptr)
    , m_subClockMeasurementPage(nullptr)
    , m_offsetMeasurementPage(nullptr)
    , m_dataManagementPage(nullptr)
    , m_flowControlPage(nullptr)
    , m_settingsPage(nullptr)
{
    setWindowTitle(QString::fromUtf8("批量测试工装工控机软件"));
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground, false);
    resize(1920, 1080);
    setMinimumSize(1380, 860);

    buildUi();
    connect(m_powerManagementPage,
            &PowerManagementPage::powerSummaryChanged,
            this,
            [this](const QString &meta, const QString &value, bool warning) {
                updateCard(QString::fromUtf8("power"), meta, value, warning);
            });
    connect(m_powerManagementPage, &PowerManagementPage::logMessage, this, &MainWindow::appendLog);
    connect(m_powerManagementPage, &PowerManagementPage::outputSettingsApplied,
            m_dataManagementPage, &DataManagementPage::appendTestRecord);

    connect(m_timingManagementPage, &TimingManagementPage::clockChanged, this, &MainWindow::handleClockChanged);
    connect(m_timingManagementPage, &TimingManagementPage::logMessage, this, &MainWindow::appendLog);
    connect(m_timingManagementPage, &TimingManagementPage::timingOutputApplied,
            m_dataManagementPage, &DataManagementPage::appendTestRecord);

    connect(m_subClockMeasurementPage, &SubClockMeasurementPage::videoStateChanged, this, &MainWindow::handleVideoStateChanged);
    connect(m_subClockMeasurementPage, &SubClockMeasurementPage::logMessage, this, &MainWindow::appendLog);

    connect(m_offsetMeasurementPage, &OffsetMeasurementPage::measurementChanged, this, &MainWindow::handleMeasurementChanged);
    connect(m_offsetMeasurementPage, &OffsetMeasurementPage::logMessage, this, &MainWindow::appendLog);

    connect(m_dataManagementPage, &DataManagementPage::storageStateChanged, this, &MainWindow::handleStorageStateChanged);
    connect(m_dataManagementPage, &DataManagementPage::logMessage, this, &MainWindow::appendLog);

    connect(m_flowControlPage, &FlowControlPage::phaseChanged, this, &MainWindow::handlePhaseChanged);
    connect(m_flowControlPage, &FlowControlPage::logMessage, this, &MainWindow::appendLog);
    connect(m_flowControlPage, &FlowControlPage::flowSessionChanged, this, &MainWindow::handleFlowSessionChanged);
    connect(m_flowControlPage, &FlowControlPage::runningStateChanged, this, &MainWindow::handleFlowRunningStateChanged);
    connect(m_flowControlPage, &FlowControlPage::stagePageChanged, this, &MainWindow::handleFlowStagePageChanged);

    connect(m_settingsPage, &SettingsPage::extraChanged, this, &MainWindow::handleExtraChanged);
    connect(m_settingsPage, &SettingsPage::logMessage, this, &MainWindow::appendLog);

    appendLog(QString::fromUtf8("批量测试工装 UI 初始化完成"));
    setCurrentModule(QString::fromUtf8("flow"));
}

MainWindow::~MainWindow()
{
}

void MainWindow::buildUi()
{
    QWidget *rootWidget = new QWidget(this);
    rootWidget->setObjectName("rootWidget");
    setCentralWidget(rootWidget);

    QVBoxLayout *outerLayout = new QVBoxLayout(rootWidget);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    QFrame *consoleFrame = new QFrame(rootWidget);
    consoleFrame->setObjectName("consoleFrame");
    outerLayout->addWidget(consoleFrame);

    QVBoxLayout *consoleLayout = new QVBoxLayout(consoleFrame);
    consoleLayout->setContentsMargins(18, 10, 18, 12);
    consoleLayout->setSpacing(22);

    consoleLayout->addWidget(buildTitleBar());

    QWidget *dashboard = new QWidget(consoleFrame);
    QHBoxLayout *dashboardLayout = new QHBoxLayout(dashboard);
    dashboardLayout->setContentsMargins(0, 0, 0, 0);
    dashboardLayout->setSpacing(0);

    QWidget *previewPanel = buildPreviewPanel();
    previewPanel->setMinimumWidth(380);
    previewPanel->setMaximumWidth(380);

    QWidget *detailPanel = buildDetailPanel();

    dashboardLayout->addWidget(previewPanel, 0);
    dashboardLayout->addWidget(detailPanel, 1);

    consoleLayout->addWidget(dashboard, 1);
}

QWidget *MainWindow::buildTitleBar()
{
    QWidget *bar = new QWidget(this);
    m_titleBarWidget = bar;
    bar->setObjectName("titleBar");
    bar->installEventFilter(this);

    QHBoxLayout *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    QWidget *titleBlock = new QWidget(bar);
    QVBoxLayout *titleLayout = new QVBoxLayout(titleBlock);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(2);

    QLabel *title = new QLabel(QString::fromUtf8("批量测试工装工控机软件"), titleBlock);
    title->setObjectName("appTitle");
    titleLayout->addWidget(title);

    QWidget *taskBar = new QWidget(bar);
    QHBoxLayout *taskLayout = new QHBoxLayout(taskBar);
    taskLayout->setContentsMargins(0, 0, 0, 0);
    taskLayout->setSpacing(10);

    m_dataButton = new QPushButton(QString::fromUtf8("数据管理"), taskBar);
    m_dataButton->setObjectName("taskButton");
    m_dataButton->setCursor(Qt::PointingHandCursor);
    m_dataButton->setMinimumHeight(46);
    m_dataButton->setMinimumWidth(150);

    QFont taskFont = m_dataButton->font();
    taskFont.setPointSize(13);
    taskFont.setBold(true);
    m_dataButton->setFont(taskFont);
    m_dataButton->setIcon(QIcon(QStringLiteral(":/image/resources/database.png")));
    m_dataButton->setIconSize(QSize(22, 22));

    m_settingsButton = new QPushButton(QString::fromUtf8("设置"), taskBar);
    m_settingsButton->setObjectName("taskButton");
    m_settingsButton->setCursor(Qt::PointingHandCursor);
    m_settingsButton->setMinimumHeight(46);
    m_settingsButton->setMinimumWidth(110);
    m_settingsButton->setFont(taskFont);
    m_settingsButton->setIcon(QIcon(QStringLiteral(":/image/resources/setting.png")));
    m_settingsButton->setIconSize(QSize(22, 22));
    connect(m_dataButton, &QPushButton::clicked, this, [this]() { switchModule(QString::fromUtf8("data")); });
    connect(m_settingsButton, &QPushButton::clicked, this, [this]() { switchModule(QString::fromUtf8("settings")); });

    taskLayout->addWidget(m_dataButton);
    taskLayout->addWidget(m_settingsButton);

    QWidget *windowButtons = new QWidget(bar);
    QHBoxLayout *windowLayout = new QHBoxLayout(windowButtons);
    windowLayout->setContentsMargins(0, 0, 0, 0);
    windowLayout->setSpacing(4);

    QPushButton *minButton = createWindowButton(QString::fromUtf8("—"), QStringLiteral("windowButton"));
    QPushButton *maxButton = createWindowButton(QString::fromUtf8("□"), QStringLiteral("windowButton"));
    QPushButton *closeButton = createWindowButton(QString::fromUtf8("x"), QStringLiteral("closeWindowButton"));

    connect(minButton, &QPushButton::clicked, this, &QWidget::showMinimized);
    connect(maxButton, &QPushButton::clicked, this, [this, maxButton]() {
        if (isMaximized()) {
            showNormal();
            maxButton->setText(QString::fromUtf8("□"));
        } else {
            showMaximized();
            maxButton->setText(QString::fromUtf8("❐"));
        }
    });
    connect(closeButton, &QPushButton::clicked, this, &QWidget::close);

    windowLayout->addWidget(minButton);
    windowLayout->addWidget(maxButton);
    windowLayout->addWidget(closeButton);

    layout->addWidget(titleBlock, 1);
    layout->addWidget(taskBar);
    layout->addWidget(windowButtons);

    return bar;
}

QWidget *MainWindow::buildPreviewPanel()
{
    QFrame *panel = new QFrame(this);
    panel->setObjectName("leftPanel");

    QVBoxLayout *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(10, 8, 10, 10);
    layout->setSpacing(8);

    QLabel *header = new QLabel(QString::fromUtf8("测试单元"), panel);
    header->setObjectName("sectionHeader");
    layout->addWidget(header);

    QScrollArea *scrollArea = new QScrollArea(panel);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    layout->addWidget(scrollArea, 1);

    QWidget *container = new QWidget(scrollArea);
    container->setObjectName("previewContainer");
    QVBoxLayout *cardsLayout = new QVBoxLayout(container);
    cardsLayout->setContentsMargins(0, 0, 0, 0);
    cardsLayout->setSpacing(10);

    const QList<ModuleInfo> modules = initialModules();
    for (int i = 0; i < modules.size(); ++i) {
        PreviewCardWidget *card = createPreviewCard(modules.at(i), container);
        cardsLayout->addWidget(card, 0);
    }

    cardsLayout->addSpacing(12);
    cardsLayout->addStretch(1);

    scrollArea->setWidget(container);

    QWidget *statusPanel = new QWidget(panel);
    statusPanel->setObjectName("leftStatusPanel");
    QVBoxLayout *statusLayout = new QVBoxLayout(statusPanel);
    statusLayout->setContentsMargins(0, 10, 0, 0);
    statusLayout->setSpacing(20);

    QLabel *flowCaptionLabel = new QLabel(QString::fromUtf8("当前流程"), statusPanel);
    flowCaptionLabel->setObjectName("leftStatusCaptionLabel");
    m_footerStateLabel = new QLabel(QString::fromUtf8("待执行"), statusPanel);
    m_footerStateLabel->setObjectName("leftStatusValueLabel");
    m_footerStateLabel->setWordWrap(true);

    QLabel *clockCaptionLabel = new QLabel(QString::fromUtf8("授时状态"), statusPanel);
    clockCaptionLabel->setObjectName("leftStatusCaptionLabel");
    m_footerClockLabel = new QLabel(QString::fromUtf8("GNSS · NTP开 · 00:00:00"), statusPanel);
    m_footerClockLabel->setObjectName("leftStatusValueLabel");
    m_footerClockLabel->setWordWrap(true);

    statusLayout->addWidget(flowCaptionLabel);
    statusLayout->addWidget(m_footerStateLabel);
    statusLayout->addSpacing(4);
    statusLayout->addWidget(clockCaptionLabel);
    statusLayout->addWidget(m_footerClockLabel);

    layout->addWidget(statusPanel, 0);

    return panel;
}

QWidget *MainWindow::buildFlowToolbar(QWidget *parent)
{
    QWidget *toolbar = new QWidget(parent);
    toolbar->setObjectName("flowToolbar");
    QHBoxLayout *layout = new QHBoxLayout(toolbar);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    m_flowToolbarPauseButton = createActionButton(QString::fromUtf8("暂停"), true, toolbar);
    QPushButton *previousButton = createActionButton(QString::fromUtf8("上一步"), false, toolbar);
    QPushButton *nextButton = createActionButton(QString::fromUtf8("下一步"), false, toolbar);
    QPushButton *retryButton = createActionButton(QString::fromUtf8("重试"), false, toolbar);
    QPushButton *endButton = createActionButton(QString::fromUtf8("结束"), false, toolbar);

    connect(m_flowToolbarPauseButton, &QPushButton::clicked, this, [this]() {
        if (!m_flowControlPage) {
            return;
        }
        if (m_flowControlPage->isRunning()) {
            m_flowControlPage->pauseProgress();
        } else {
            m_flowControlPage->startProgress();
        }
    });
    connect(previousButton, &QPushButton::clicked, m_flowControlPage, &FlowControlPage::previousStage);
    connect(nextButton, &QPushButton::clicked, m_flowControlPage, &FlowControlPage::nextStage);
    connect(retryButton, &QPushButton::clicked, m_flowControlPage, &FlowControlPage::retryCurrentStage);
    connect(endButton, &QPushButton::clicked, m_flowControlPage, &FlowControlPage::endFlow);

    layout->addWidget(m_flowToolbarPauseButton);
    layout->addWidget(previousButton);
    layout->addWidget(nextButton);
    layout->addWidget(retryButton);
    layout->addWidget(endButton);
    layout->addStretch();

    toolbar->setVisible(false);
    return toolbar;
}

QWidget *MainWindow::buildDetailPanel()
{
    QFrame *panel = new QFrame(this);
    panel->setObjectName("rightPanel");

    QVBoxLayout *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(10);

    QFrame *headerFrame = new QFrame(panel);
    headerFrame->setObjectName("detailHeaderFrame");
    QVBoxLayout *headerLayout = new QVBoxLayout(headerFrame);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(0);

    m_detailTitleLabel = new QLabel(QString::fromUtf8("测试流程控制"), headerFrame);
    m_detailTitleLabel->setObjectName("detailPanelTitle");
    headerLayout->addWidget(m_detailTitleLabel);
    layout->addWidget(headerFrame);

    m_flowControlPage = new FlowControlPage(panel);
    m_flowToolbar = buildFlowToolbar(panel);
    layout->addWidget(m_flowToolbar);

    m_stackedWidget = new QStackedWidget(panel);

    m_timingManagementPage = new TimingManagementPage(m_stackedWidget);
    m_offsetMeasurementPage = new OffsetMeasurementPage(m_stackedWidget);
    m_powerManagementPage = new PowerManagementPage(m_stackedWidget);
    m_subClockMeasurementPage = new SubClockMeasurementPage(m_stackedWidget);
    m_dataManagementPage = new DataManagementPage(m_stackedWidget);
    m_settingsPage = new SettingsPage(m_stackedWidget);

    m_pageIndex.insert(QString::fromUtf8("flow"), m_stackedWidget->addWidget(m_flowControlPage));
    m_pageIndex.insert(QString::fromUtf8("timing"), m_stackedWidget->addWidget(m_timingManagementPage));
    m_pageIndex.insert(QString::fromUtf8("offset"), m_stackedWidget->addWidget(m_offsetMeasurementPage));
    m_pageIndex.insert(QString::fromUtf8("power"), m_stackedWidget->addWidget(m_powerManagementPage));
    m_pageIndex.insert(QString::fromUtf8("subclock"), m_stackedWidget->addWidget(m_subClockMeasurementPage));
    m_pageIndex.insert(QString::fromUtf8("data"), m_stackedWidget->addWidget(m_dataManagementPage));
    m_pageIndex.insert(QString::fromUtf8("settings"), m_stackedWidget->addWidget(m_settingsPage));

    layout->addWidget(m_stackedWidget, 1);

    return panel;
}

PreviewCardWidget *MainWindow::createPreviewCard(const ModuleInfo &info, QWidget *parent)
{
    PreviewCardWidget *card = new PreviewCardWidget(info.id,
                                                    info.iconPath,
                                                    info.title,
                                                    info.meta,
                                                    info.value,
                                                    info.accentColor,
                                                    parent);
    card->setLedState(info.warning ? PreviewCardWidget::LedWarning : PreviewCardWidget::LedOk);
    connect(card, &PreviewCardWidget::clicked, this, &MainWindow::switchModule);
    m_cards.insert(info.id, card);
    return card;
}

void MainWindow::updateCard(const QString &id, const QString &meta, const QString &value, bool warning)
{
    if (!m_cards.contains(id)) {
        return;
    }

    PreviewCardWidget *card = m_cards.value(id);
    card->setMetaText(meta);
    card->setValueText(value);
    card->setLedState(warning ? PreviewCardWidget::LedWarning : PreviewCardWidget::LedOk);
}

void MainWindow::setCurrentModule(const QString &id)
{
    if (!m_pageIndex.contains(id)) {
        return;
    }

    const QList<QString> keys = m_cards.keys();
    for (int i = 0; i < keys.size(); ++i) {
        m_cards.value(keys.at(i))->setActive(keys.at(i) == id);
    }

    m_stackedWidget->setCurrentIndex(m_pageIndex.value(id));
    updateTaskButtonState(m_dataButton, id == QString::fromUtf8("data"));
    updateTaskButtonState(m_settingsButton, id == QString::fromUtf8("settings"));
    updateDetailHeader(id);
}

void MainWindow::updateTaskButtonState(QPushButton *button, bool active)
{
    if (!button) {
        return;
    }

    button->setProperty("active", active);
    button->style()->unpolish(button);
    button->style()->polish(button);
    button->update();
}

void MainWindow::updateDetailHeader(const QString &id, const QString &overrideTitle)
{
    if (!overrideTitle.isEmpty()) {
        m_detailTitleLabel->setText(overrideTitle);
        return;
    }

    if (id == QString::fromUtf8("flow")) {
        m_detailTitleLabel->setText(QString::fromUtf8("测试流程控制"));
    } else if (id == QString::fromUtf8("timing")) {
        m_detailTitleLabel->setText(QString::fromUtf8("授时管理"));
    } else if (id == QString::fromUtf8("offset")) {
        m_detailTitleLabel->setText(QString::fromUtf8("时差测量"));
    } else if (id == QString::fromUtf8("power")) {
        m_detailTitleLabel->setText(QString::fromUtf8("电源管理"));
    } else if (id == QString::fromUtf8("subclock")) {
        m_detailTitleLabel->setText(QString::fromUtf8("子钟时差测量"));
    } else if (id == QString::fromUtf8("data")) {
        m_detailTitleLabel->setText(QString::fromUtf8("数据管理"));
    } else if (id == QString::fromUtf8("settings")) {
        m_detailTitleLabel->setText(QString::fromUtf8("设置"));
    }
}

QList<ModuleInfo> MainWindow::initialModules() const
{
    QList<ModuleInfo> modules;
    modules << ModuleInfo{QString::fromUtf8("flow"),
                          QStringLiteral(":/image/resources/flow.png"),
                          QString::fromUtf8("测试流程控制"),
                          QString::fromUtf8("单元编排 / 顺序 / 重试 / 报告"),
                          QString::fromUtf8("待执行"),
                          QStringLiteral("#2c67ff"),
                          false};

    modules << ModuleInfo{QString::fromUtf8("timing"),
                          QStringLiteral(":/image/resources/timing.png"),
                          QString::fromUtf8("授时管理"),
                          QString::fromUtf8("授时源 / NTP / 5路输出"),
                          QString::fromUtf8("等待数据"),
                          QStringLiteral("#14b8a6"),
                          false};

    modules << ModuleInfo{QString::fromUtf8("offset"),
                          QStringLiteral(":/image/resources/offset.png"),
                          QString::fromUtf8("时差测量"),
                          QString::fromUtf8("5路通道测量 / 趋势 / 稳定度"),
                          QString::fromUtf8("未开始"),
                          QStringLiteral("#7c3aed"),
                          false};

    modules << ModuleInfo{QString::fromUtf8("power"),
                          QStringLiteral(":/image/resources/power.png"),
                          QString::fromUtf8("电源管理"),
                          QString::fromUtf8("输出关闭 / 设备待机"),
                          QString::fromUtf8("OFF"),
                          QStringLiteral("#ea580c"),
                          true};

    modules << ModuleInfo{QString::fromUtf8("subclock"),
                          QStringLiteral(":/image/resources/subclock.png"),
                          QString::fromUtf8("子钟时差测量"),
                          QString::fromUtf8("在线状态 / 当前阶段 / 证据图"),
                          QString::fromUtf8("待命"),
                          QStringLiteral("#0ea5e9"),
                          false};
    return modules;
}

void MainWindow::switchModule(const QString &moduleId)
{
    setCurrentModule(moduleId);
}

void MainWindow::handlePowerStateChanged(bool on)
{
    Q_UNUSED(on);
}
void MainWindow::handleClockChanged(const QString &clockText)
{
    QString shortText = QString::fromUtf8("等待数据");

    if (clockText.contains(QString::fromUtf8("1PPS + ToD"))) {
        shortText = QString::fromUtf8("1PPS+ToD");
    } else if (clockText.contains(QString::fromUtf8("GNSS"))) {
        shortText = QString::fromUtf8("GNSS");
    }

    updateCard(QString::fromUtf8("timing"),
               QString::fromUtf8("授时源 / NTP / 5路输出"),
               shortText,
               false);

    if (m_footerClockLabel) {
        m_footerClockLabel->setText(clockText);
    }
}

void MainWindow::handleVideoStateChanged(bool online)
{
    updateCard(QString::fromUtf8("subclock"),
               online ? QString::fromUtf8("在线 / 拍摄阶段推进中") : QString::fromUtf8("离线 / 等待测试架连接"),
               online ? QString::fromUtf8("8路子钟") : QString::fromUtf8("等待连接"),
               !online);
}

void MainWindow::handleMeasurementChanged(const QString &summary)
{
    updateCard(QString::fromUtf8("offset"),
               QString::fromUtf8("5路偏差概览 / 平均稳定度"),
               summary,
               summary.contains(QString::fromUtf8("告警")));
}

void MainWindow::handleStorageStateChanged(const QString &meta, const QString &value, const QString &writeRate)
{
    if (m_dataButton) {
        m_dataButton->setToolTip(meta + QString::fromUtf8(" · ") + value);
    }

    if (m_footerWriteRateLabel) {
        m_footerWriteRateLabel->setText(writeRate);
    }

    const bool idle = meta.contains(QString::fromUtf8("待机"))
                      || meta.contains(QString::fromUtf8("暂停"))
                      || writeRate.contains(QString::fromUtf8("0MB/s"))
                      || writeRate.contains(QString::fromUtf8("0 MB/s"));

    m_footerStateLabel->setText(idle
                                ? QString::fromUtf8("数据采集已暂停")
                                : QString::fromUtf8("数据采集运行中"));

    updateCard(QString::fromUtf8("data"),
               idle ? QString::fromUtf8("数据库待机 / 日志可导出")
                    : QString::fromUtf8("数据库在线 / 自动采集中"),
               idle ? QString::fromUtf8("待机")
                    : writeRate,
               false);
}

void MainWindow::handlePhaseChanged(const QString &meta, const QString &value)
{
    QString shortValue = QString::fromUtf8("待执行");

    if (value.contains(QString::fromUtf8("完成"))) {
        shortValue = QString::fromUtf8("当前单元完成");
    } else if (value.contains(QString::fromUtf8("等待开始"))) {
        shortValue = QString::fromUtf8("待开始");
    } else {
        shortValue = QString::fromUtf8("执行中");
    }

    updateCard(QString::fromUtf8("flow"),
               QString::fromUtf8("单元编排 / 顺序 / 重试 / 报告"),
               shortValue,
               false);

    m_footerStateLabel->setText(QString::fromUtf8("%1 · %2").arg(meta, value));
}

void MainWindow::handleExtraChanged(const QString &value)
{
    if (m_settingsButton) {
        m_settingsButton->setToolTip(value);
    }
}

void MainWindow::handleFlowSessionChanged(bool active)
{
    if (m_flowToolbar) {
        m_flowToolbar->setVisible(active);
    }
    if (m_flowToolbarPauseButton) {
        m_flowToolbarPauseButton->setText(QString::fromUtf8("暂停"));
    }

    if (!active) {
        setCurrentModule(QString::fromUtf8("flow"));
        updateDetailHeader(QString::fromUtf8("flow"));
    }
}

void MainWindow::handleFlowRunningStateChanged(bool running)
{
    if (m_flowToolbarPauseButton) {
        m_flowToolbarPauseButton->setText(running ? QString::fromUtf8("暂停")
                                                  : QString::fromUtf8("继续"));
    }
}

void MainWindow::handleFlowStagePageChanged(const QString &moduleId, const QString &title)
{
    if (!m_pageIndex.contains(moduleId)) {
        return;
    }

    setCurrentModule(moduleId);
    updateDetailHeader(moduleId, title);
}

void MainWindow::appendLog(const QString &message)
{
    if (m_dataManagementPage) {
        m_dataManagementPage->appendRuntimeLog(message);
    }
    if (message.contains(QString::fromUtf8("告警"))) {
        m_footerStateLabel->setText(QString::fromUtf8("存在告警事件"));
    }
}

QPushButton *MainWindow::createWindowButton(const QString &text, const QString &objectName)
{
    QPushButton *button = new QPushButton(text, this);
    button->setObjectName(objectName);
    button->setCursor(Qt::PointingHandCursor);
    button->setFixedSize(68, 48);
    return button;
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_titleBarWidget) {
        if (event->type() == QEvent::MouseButtonDblClick) {
            if (isMaximized()) {
                showNormal();
            } else {
                showMaximized();
            }
            return true;
        }

        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                m_draggingWindow = true;
                m_dragOffset = mouseEvent->globalPos() - frameGeometry().topLeft();
                return true;
            }
        } else if (event->type() == QEvent::MouseMove && m_draggingWindow && !isMaximized()) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            move(mouseEvent->globalPos() - m_dragOffset);
            return true;
        } else if (event->type() == QEvent::MouseButtonRelease) {
            m_draggingWindow = false;
            return true;
        }
    }

    return QMainWindow::eventFilter(watched, event);
}
