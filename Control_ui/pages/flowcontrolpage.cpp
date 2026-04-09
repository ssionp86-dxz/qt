#include "flowcontrolpage.h"

#include "detailuifactory.h"

#include <QAbstractItemView>
#include <QColor>
#include <QFrame>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QProgressBar>
#include <QPushButton>
#include <QSizePolicy>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>

using namespace DetailUiFactory;

FlowControlPage::FlowControlPage(QWidget *parent)
    : QWidget(parent)
    , m_stageOrder(QVector<int>() << 1 << 2 << 3 << 4 << 5)
    , m_currentStage(1)
    , m_progress(0)
    , m_running(false)
    , m_flowSessionActive(false)
    , m_stageExecutionCounts(QVector<int>(5, 0))
    , m_completedStages()
    , m_currentStageCounted(false)
    , m_stageValue(nullptr)
    , m_progressValue(nullptr)
    , m_stepValue(nullptr)
    , m_progressBar(nullptr)
    , m_flowTable(nullptr)
    , m_stepList(nullptr)
    , m_phaseTimer(new QTimer(this))
    , m_preStartControls(nullptr)
    , m_inlineStartButton(nullptr)
{
    setObjectName("flowControlPage");

    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    QFrame *card = createCardFrame(this);
    rootLayout->addWidget(card);

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(18);

    QWidget *topButtons = new QWidget(card);
    QHBoxLayout *topButtonLayout = new QHBoxLayout(topButtons);
    topButtonLayout->setContentsMargins(0, 0, 0, 0);
    topButtonLayout->setSpacing(12);

    m_inlineStartButton = createActionButton(QString::fromUtf8("开始流程"), true, topButtons);
    QPushButton *loadButton = createActionButton(QString::fromUtf8("加载历史流程"), false, topButtons);
    QPushButton *saveButton = createActionButton(QString::fromUtf8("保存当前流程"), false, topButtons);
    QPushButton *moveUpButton = createActionButton(QString::fromUtf8("上移"), false, topButtons);
    QPushButton *moveDownButton = createActionButton(QString::fromUtf8("下移"), false, topButtons);

    connect(loadButton, &QPushButton::clicked, this, &FlowControlPage::loadHistory);
    connect(m_inlineStartButton, &QPushButton::clicked, this, &FlowControlPage::startProgress);
    connect(saveButton, &QPushButton::clicked, this, &FlowControlPage::saveFlow);
    connect(moveUpButton, &QPushButton::clicked, this, &FlowControlPage::moveStageUp);
    connect(moveDownButton, &QPushButton::clicked, this, &FlowControlPage::moveStageDown);

    topButtonLayout->addWidget(m_inlineStartButton);
    topButtonLayout->addWidget(loadButton);
    topButtonLayout->addWidget(saveButton);
    topButtonLayout->addWidget(moveUpButton);
    topButtonLayout->addWidget(moveDownButton);
    topButtonLayout->addStretch();
    layout->addWidget(topButtons);

    QWidget *contentRow = new QWidget(card);
    QHBoxLayout *contentLayout = new QHBoxLayout(contentRow);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(16);

    QWidget *leftPanel = new QWidget(contentRow);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(12);

    m_flowTable = new QTableWidget(5, 5, leftPanel);
    m_flowTable->setObjectName("pageTable");
    m_flowTable->setHorizontalHeaderLabels(QStringList()
                                           << QString::fromUtf8("顺序")
                                           << QString::fromUtf8("测试单元")
                                           << QString::fromUtf8("执行次数")
                                           << QString::fromUtf8("说明")
                                           << QString::fromUtf8("状态"));
    QFont flowTableFont = m_flowTable->font();
    flowTableFont.setPointSize(16);
    flowTableFont.setBold(true);
    m_flowTable->setFont(flowTableFont);

    m_flowTable->verticalHeader()->setVisible(false);
    m_flowTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_flowTable->horizontalHeader()->setMinimumHeight(64);
    m_flowTable->verticalHeader()->setDefaultSectionSize(112);

    m_flowTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_flowTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_flowTable->setSelectionMode(QAbstractItemView::SingleSelection);

    m_flowTable->setMinimumHeight(640);
    leftPanel->setMinimumWidth(880);

    leftLayout->addWidget(m_flowTable, 1);
    connect(m_flowTable, &QTableWidget::itemSelectionChanged,
            this, &FlowControlPage::applySelectedStage);

    QWidget *rightPanel = new QWidget(contentRow);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(8);

    auto createFlowInfoRow = [rightPanel](const QString &labelText, const QString &valueText, QLabel **valueOut) {
        QWidget *row = new QWidget(rightPanel);
        rightPanel->setMinimumWidth(420);
        row->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        QHBoxLayout *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 1, 0, 1);
        rowLayout->setSpacing(10);

        QLabel *label = new QLabel(labelText, row);
        label->setObjectName("flowInfoLabel");

        QLabel *value = new QLabel(valueText, row);
        value->setObjectName("flowInfoValue");
        value->setAlignment(Qt::AlignCenter);
        value->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

        rowLayout->addWidget(label);
        rowLayout->addStretch();
        rowLayout->addWidget(value);

        if (valueOut) {
            *valueOut = value;
        }
        return row;
    };

    rightLayout->addWidget(createFlowInfoRow(QString::fromUtf8("当前测试单元"), QString::fromUtf8("主钟电源适应性测试"), &m_stageValue));
    rightLayout->addWidget(createFlowInfoRow(QString::fromUtf8("当前子步骤"), QString::fromUtf8("等待开始"), &m_stepValue));
    rightLayout->addWidget(createFlowInfoRow(QString::fromUtf8("总进度"), QString::fromUtf8("0%"), &m_progressValue));

    m_progressBar = createThinProgress(QStringLiteral("#3b82f6"), rightPanel);
    rightLayout->addWidget(m_progressBar);

    QLabel *stepsTitle = new QLabel(QString::fromUtf8("当前单元子步骤"), rightPanel);
    stepsTitle->setObjectName("flowStepsTitle");
    stepsTitle->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    rightLayout->addWidget(stepsTitle);

    m_stepList = new QListWidget(rightPanel);
    m_stepList->setObjectName("simpleList");
    m_stepList->setMinimumHeight(220);
    rightLayout->addWidget(m_stepList, 1);

    connect(m_phaseTimer, &QTimer::timeout, this, &FlowControlPage::advanceProgress);

    contentLayout->addWidget(leftPanel, 5);
    contentLayout->addWidget(rightPanel, 3);

    layout->addWidget(contentRow, 1);

    if (!m_stageOrder.isEmpty()) {
        m_currentStage = m_stageOrder.first();
    }
    refreshUi();
    syncSelection();
}

QString FlowControlPage::currentStageModuleId() const
{
    return stageModuleId(m_currentStage);
}

bool FlowControlPage::flowSessionActive() const
{
    return m_flowSessionActive;
}

bool FlowControlPage::isRunning() const
{
    return m_running;
}

void FlowControlPage::startProgress()
{
    if (m_stageOrder.isEmpty()) {
        return;
    }

    int selectedRow = currentSelectedRow();
    if (selectedRow < 0 || selectedRow >= m_stageOrder.size()) {
        selectedRow = currentStageRow();
    }
    if (selectedRow < 0 || selectedRow >= m_stageOrder.size()) {
        selectedRow = 0;
    }

    if (!m_flowSessionActive) {
        m_flowSessionActive = true;
        emit flowSessionChanged(true);
    }

    if (!m_running && m_progress == 0) {
        m_currentStage = m_stageOrder.at(selectedRow);
        m_currentStageCounted = false;
        m_completedStages.remove(m_currentStage);
        syncSelection();
        refreshUi();
        notifyStagePageChanged();
    }

    if (!m_phaseTimer->isActive()) {
        m_phaseTimer->start(850);
        m_running = true;
        emit runningStateChanged(true);
        refreshUi();
        emit logMessage(QString::fromUtf8("测试流程：开始执行“%1”").arg(stageName(m_currentStage)));
    }
}

void FlowControlPage::pauseProgress()
{
    m_phaseTimer->stop();
    m_running = false;
    emit runningStateChanged(false);
    refreshUi();
    emit logMessage(QString::fromUtf8("测试流程：已暂停当前测试单元"));
}

void FlowControlPage::nextStage()
{
    m_phaseTimer->stop();
    m_running = false;
    emit runningStateChanged(false);

    if (m_stageOrder.isEmpty()) {
        refreshUi();
        return;
    }

    markCurrentStageCompleted();

    int currentRow = currentStageRow();
    if (currentRow < 0) {
        currentRow = 0;
    }

    int nextRow = currentRow + 1;
    if (nextRow >= m_stageOrder.size()) {
        m_completedStages.clear();
        nextRow = 0;
        emit logMessage(QString::fromUtf8("测试流程：已到最后一步，已循环返回第一步"));
    } else {
        emit logMessage(QString::fromUtf8("测试流程：切换到“%1”").arg(stageName(m_stageOrder.at(nextRow))));
    }

    m_currentStage = m_stageOrder.at(nextRow);
    m_progress = 0;
    m_currentStageCounted = false;

    syncSelection();
    refreshUi();
    notifyStagePageChanged();
}

void FlowControlPage::previousStage()
{
    m_phaseTimer->stop();
    m_running = false;
    emit runningStateChanged(false);

    const int currentRow = currentStageRow();
    if (currentRow > 0) {
        m_currentStage = m_stageOrder.at(currentRow - 1);
        m_progress = 0;
        m_currentStageCounted = false;
        syncSelection();
        emit logMessage(QString::fromUtf8("测试流程：回退到“%1”").arg(stageName(m_currentStage)));
        notifyStagePageChanged();
    }

    refreshUi();
}

void FlowControlPage::retryCurrentStage()
{
    m_phaseTimer->stop();
    m_running = false;
    emit runningStateChanged(false);
    m_progress = 0;
    m_currentStageCounted = false;
    m_completedStages.remove(m_currentStage);
    refreshUi();
    notifyStagePageChanged();
    emit logMessage(QString::fromUtf8("测试流程：已重试当前测试单元“%1”").arg(stageName(m_currentStage)));
}

void FlowControlPage::endFlow()
{
    m_phaseTimer->stop();
    m_running = false;
    emit runningStateChanged(false);
    m_progress = 100;
    markCurrentStageCompleted();

    if (m_flowSessionActive) {
        m_flowSessionActive = false;
        emit flowSessionChanged(false);
    }

    refreshUi();
    emit stagePageChanged(QString::fromUtf8("flow"), QString::fromUtf8("测试流程控制"));
    emit logMessage(QString::fromUtf8("测试流程：全部流程结束，等待导出测试报告"));
}

void FlowControlPage::loadHistory()
{
    emit logMessage(QString::fromUtf8("测试流程：已加载历史流程模板（批量模式）"));
}

void FlowControlPage::saveFlow()
{
    emit logMessage(QString::fromUtf8("测试流程：当前流程已保存为历史模板"));
}

void FlowControlPage::advanceProgress()
{
    m_progress += 8;
    if (m_progress >= 100) {
        m_progress = 100;
        m_phaseTimer->stop();
        m_running = false;
        emit runningStateChanged(false);
        markCurrentStageCompleted();
        emit logMessage(QString::fromUtf8("测试流程：测试单元“%1”执行完成").arg(stageName(m_currentStage)));
    }
    refreshUi();
}

int FlowControlPage::currentSelectedRow() const
{
    if (!m_flowTable || !m_flowTable->selectionModel() || m_flowTable->selectionModel()->selectedRows().isEmpty()) {
        return -1;
    }
    return m_flowTable->selectionModel()->selectedRows().first().row();
}

QString FlowControlPage::stageDescription(int stageId) const
{
    switch (stageId) {
    case 1:
        return QString::fromUtf8("交流/直流电源适应性");
    case 2:
        return QString::fromUtf8("初始化守时精度");
    case 3:
        return QString::fromUtf8("授时精度与源切换");
    case 4:
        return QString::fromUtf8("视频拍摄与识别");
    case 5:
        return QString::fromUtf8("24h 守时与复测");
    default:
        return QString::fromUtf8("未知说明");
    }
}

QString FlowControlPage::stageExecutionText(int stageId) const
{
    const int count = (stageId >= 1 && stageId <= m_stageExecutionCounts.size())
            ? m_stageExecutionCounts.at(stageId - 1)
            : 0;

    return stageId == 5
            ? QString::fromUtf8("%1 次 / 24h").arg(count)
            : QString::fromUtf8("%1 次").arg(count);
}

QString FlowControlPage::stageModuleId(int stageId) const
{
    switch (stageId) {
    case 1:
        return QString::fromUtf8("power");
    case 2:
        return QString::fromUtf8("offset");
    case 3:
        return QString::fromUtf8("timing");
    case 4:
        return QString::fromUtf8("subclock");
    case 5:
        return QString::fromUtf8("offset");
    default:
        return QString::fromUtf8("flow");
    }
}

void FlowControlPage::swapStages(int firstRow, int secondRow)
{
    if (firstRow < 0 || secondRow < 0
        || firstRow >= m_stageOrder.size()
        || secondRow >= m_stageOrder.size()) {
        return;
    }

    qSwap(m_stageOrder[firstRow], m_stageOrder[secondRow]);

    int selectedRow = currentSelectedRow();
    if (selectedRow < 0) {
        selectedRow = qMin(firstRow, secondRow);
    }

    if (selectedRow >= 0 && selectedRow < m_stageOrder.size()) {
        m_currentStage = m_stageOrder.at(selectedRow);
    } else if (!m_stageOrder.isEmpty()) {
        m_currentStage = m_stageOrder.first();
    }

    updateFlowTable();
    syncSelection();
    refreshUi();
}

void FlowControlPage::moveStageUp()
{
    if (m_running) {
        emit logMessage(QString::fromUtf8("测试流程：执行中禁止调整流程顺序"));
        return;
    }

    const int row = currentSelectedRow();
    if (row > 0) {
        swapStages(row, row - 1);
        m_flowTable->selectRow(row - 1);
        emit logMessage(QString::fromUtf8("测试流程：已上移当前测试单元"));
    }
}

void FlowControlPage::moveStageDown()
{
    if (m_running) {
        emit logMessage(QString::fromUtf8("测试流程：执行中禁止调整流程顺序"));
        return;
    }

    const int row = currentSelectedRow();
    if (row >= 0 && row < m_stageOrder.size() - 1) {
        swapStages(row, row + 1);
        m_flowTable->selectRow(row + 1);
        emit logMessage(QString::fromUtf8("测试流程：已下移当前测试单元"));
    }
}

void FlowControlPage::applySelectedStage()
{
    if (m_running) {
        return;
    }

    const int row = currentSelectedRow();
    if (row >= 0 && row < m_stageOrder.size()) {
        m_currentStage = m_stageOrder.at(row);
        m_progress = 0;
        m_currentStageCounted = false;
        refreshUi();
        if (m_flowSessionActive) {
            notifyStagePageChanged();
        }
        emit logMessage(QString::fromUtf8("测试流程：已选择测试单元“%1”").arg(stageName(m_currentStage)));
    }
}

void FlowControlPage::syncSelection()
{
    if (!m_flowTable) {
        return;
    }

    const int row = m_stageOrder.indexOf(m_currentStage);
    if (row < 0) {
        return;
    }

    m_flowTable->blockSignals(true);
    m_flowTable->clearSelection();
    m_flowTable->selectRow(row);
    m_flowTable->blockSignals(false);
}

QString FlowControlPage::stageName(int stage) const
{
    switch (stage) {
    case 1:
        return QString::fromUtf8("主钟电源适应性测试");
    case 2:
        return QString::fromUtf8("主钟守时精度测量");
    case 3:
        return QString::fromUtf8("主钟授时精度测量");
    case 4:
        return QString::fromUtf8("子钟时差测量");
    case 5:
        return QString::fromUtf8("子钟守时精度测量");
    default:
        return QString::fromUtf8("未知测试单元");
    }
}

QStringList FlowControlPage::stageSteps(int stage) const
{
    switch (stage) {
    case 1:
        return QStringList()
            << QString::fromUtf8("交流电源适应性测试")
            << QString::fromUtf8("220V电源电压拉偏设置 [150V,286V]")
            << QString::fromUtf8("AC220V 频率拉偏设置 [45Hz,55Hz]")
            << QString::fromUtf8("直流 24V 电源适应性测试")
            << QString::fromUtf8("电源输出拉偏 [16V,32V]");
    case 2:
        return QStringList()
            << QString::fromUtf8("初始精度测量")
            << QString::fromUtf8("设置测量时间")
            << QString::fromUtf8("测试端口设置和选择")
            << QString::fromUtf8("记录首次结果");
    case 3:
        return QStringList()
            << QString::fromUtf8("授时源状态检查")
            << QString::fromUtf8("1PPS 输出相位补偿设置")
            << QString::fromUtf8("NTP 服务状态检查")
            << QString::fromUtf8("授时精度记录");
    case 4:
        return QStringList()
            << QString::fromUtf8("子钟视频拍摄编辑控制")
            << QString::fromUtf8("视频拍摄流程控制")
            << QString::fromUtf8("视频拍摄结果记录");
    case 5:
        return QStringList()
            << QString::fromUtf8("24h 守时精度测量")
            << QString::fromUtf8("复测并比对前后误差")
            << QString::fromUtf8("导出守时精度结果");
    default:
        return QStringList();
    }
}

void FlowControlPage::refreshUi()
{
    const QStringList steps = stageSteps(m_currentStage);
    const int stepCount = qMax(1, steps.size());
    const int stepIndex = qMin(stepCount - 1, (m_progress * stepCount) / 100);

    QString stepText;
    if (m_running) {
        stepText = steps.value(stepIndex, QString::fromUtf8("执行中"));
    } else if (m_progress >= 100) {
        stepText = QString::fromUtf8("当前单元完成");
    } else {
        stepText = QString::fromUtf8("等待开始");
    }

    m_stageValue->setText(stageName(m_currentStage));
    m_stepValue->setText(stepText);

    const int currentRow = qMax(0, m_stageOrder.indexOf(m_currentStage));
    const int totalCount = qMax(1, m_stageOrder.size());
    const int overallProgress = qMin(100, (currentRow * 100 + m_progress) / totalCount);

    m_progressValue->setText(QString::fromUtf8("%1%").arg(overallProgress));
    m_progressBar->setValue(m_progress);

    m_stepList->clear();
    for (int i = 0; i < steps.size(); ++i) {
        QListWidgetItem *item = new QListWidgetItem(steps.at(i), m_stepList);
        if (m_running && i == stepIndex) {
            item->setBackground(QColor("#1e3a8a"));
            item->setForeground(QColor("#ffffff"));
        }
    }

    if (m_inlineStartButton) {
        m_inlineStartButton->setVisible(!m_flowSessionActive);
    }

    updateFlowTable();
    syncSelection();
    emit phaseChanged(stageName(m_currentStage), stepText);
}

void FlowControlPage::updateFlowTable()
{
    m_flowTable->setRowCount(m_stageOrder.size());

    for (int row = 0; row < m_stageOrder.size(); ++row) {
        const int stageId = m_stageOrder.at(row);

        QString status = QString::fromUtf8("待执行");

        if (stageId == m_currentStage) {
            if (m_running) {
                status = QString::fromUtf8("执行中");
            } else if (m_progress >= 100 || m_completedStages.contains(stageId)) {
                status = QString::fromUtf8("已完成");
            } else {
                status = QString::fromUtf8("待开始");
            }
        } else if (m_completedStages.contains(stageId)) {
            status = QString::fromUtf8("已完成");
        }

        m_flowTable->setItem(row, 0, new QTableWidgetItem(QString::number(row + 1)));
        m_flowTable->setItem(row, 1, new QTableWidgetItem(stageName(stageId)));
        m_flowTable->setItem(row, 2, new QTableWidgetItem(stageExecutionText(stageId)));
        m_flowTable->setItem(row, 3, new QTableWidgetItem(stageDescription(stageId)));
        m_flowTable->setItem(row, 4, new QTableWidgetItem(status));

        for (int col = 0; col < 5; ++col) {
            if (QTableWidgetItem *item = m_flowTable->item(row, col)) {
                item->setTextAlignment(col == 1 || col == 3
                                           ? Qt::AlignVCenter | Qt::AlignLeft
                                           : Qt::AlignCenter);
            }
        }
    }
}

int FlowControlPage::currentStageRow() const
{
    return m_stageOrder.indexOf(m_currentStage);
}

void FlowControlPage::markCurrentStageCompleted()
{
    if (m_currentStage < 1 || m_currentStage > m_stageExecutionCounts.size()) {
        return;
    }

    if (!m_currentStageCounted) {
        ++m_stageExecutionCounts[m_currentStage - 1];
        m_currentStageCounted = true;
    }

    m_completedStages.insert(m_currentStage);
}

void FlowControlPage::notifyStagePageChanged()
{
    emit stagePageChanged(stageModuleId(m_currentStage), stageName(m_currentStage));
}
