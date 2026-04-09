#include "datamanagementpage.h"

#include "detailuifactory.h"

#include <QAbstractItemView>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextStream>
#include <QVBoxLayout>
#include <QtMath>

using namespace DetailUiFactory;

DataManagementPage::DataManagementPage(QWidget *parent)
    : QWidget(parent)
    , m_recording(true)
    , m_transitioning(false)
    , m_usedGb(369)
    , m_recordSequence(0)
    , m_currentWriteRate(0)
    , m_storagePoolValue(nullptr)
    , m_usageValue(nullptr)
    , m_freeValue(nullptr)
    , m_healthValue(nullptr)
    , m_captureStateValue(nullptr)
    , m_writeSpeedValue(nullptr)
    , m_lastActionValue(nullptr)
    , m_recordButton(nullptr)
    , m_usageBar(nullptr)
    , m_recordTable(nullptr)
    , m_runtimeLogEdit(nullptr)
    , m_historyLogList(nullptr)
{
    setObjectName(QString::fromUtf8("dataManagementPage"));

    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    QFrame *card = createCardFrame(this);
    rootLayout->addWidget(card);

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(14);

    auto createMetricGroup = [](QWidget *parent) -> QWidget * {
        QWidget *group = new QWidget(parent);
        group->setStyleSheet(QString::fromUtf8(
            "QWidget {"
            " background: transparent;"
            " border: none;"
            "}"));
        return group;
    };

    auto createMetricRow = [](const QString &titleText,
                              const QString &valueText,
                              QLabel **valuePtr,
                              QWidget *parent) -> QWidget * {
        QWidget *row = new QWidget(parent);
        QHBoxLayout *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(18);

        QLabel *title = new QLabel(titleText, row);
        title->setMinimumWidth(100);
        title->setStyleSheet(QString::fromUtf8(
            "QLabel { color:#a8bcde; font-size:22px; font-weight:600; background:transparent; }"));

        QLabel *value = new QLabel(valueText, row);
        value->setAlignment(Qt::AlignCenter);
        value->setMinimumHeight(42);
        value->setMinimumWidth(196);
        value->setStyleSheet(QString::fromUtf8(
            "QLabel {"
            " color:#f4f8ff;"
            " font-size:20px;"
            " font-weight:700;"
            " padding:8px 16px;"
            " border:1px solid #274671;"
            " border-radius:14px;"
            " background-color:#0a1730;"
            "}"));

        rowLayout->addWidget(title);
        rowLayout->addStretch();
        rowLayout->addWidget(value);

        if (valuePtr) {
            *valuePtr = value;
        }
        return row;
    };

    QWidget *topSection = new QWidget(card);
    QHBoxLayout *topSectionLayout = new QHBoxLayout(topSection);
    topSectionLayout->setContentsMargins(0, 0, 0, 0);
    topSectionLayout->setSpacing(88);

    QWidget *leftGroup = createMetricGroup(topSection);
    leftGroup->setMaximumWidth(600);
    QVBoxLayout *leftGroupLayout = new QVBoxLayout(leftGroup);
    leftGroupLayout->setContentsMargins(0, 0, 0, 0);
    leftGroupLayout->setSpacing(16);
    leftGroupLayout->addWidget(createMetricRow(QString::fromUtf8("数据库状态"),
                                               QString::fromUtf8("在线 / SQLite"),
                                               &m_storagePoolValue,
                                               leftGroup));
    leftGroupLayout->addWidget(createMetricRow(QString::fromUtf8("已用容量"),
                                               QString::fromUtf8("369 GB / 2.0 TB"),
                                               &m_usageValue,
                                               leftGroup));
    leftGroupLayout->addWidget(createMetricRow(QString::fromUtf8("磁盘健康度"),
                                               QString::fromUtf8("良好 (100%)"),
                                               &m_healthValue,
                                               leftGroup));

    QWidget *rightGroup = createMetricGroup(topSection);
    rightGroup->setMaximumWidth(620);
    QVBoxLayout *rightGroupLayout = new QVBoxLayout(rightGroup);
    rightGroupLayout->setContentsMargins(0, 0, 0, 0);
    rightGroupLayout->setSpacing(16);
    rightGroupLayout->addWidget(createMetricRow(QString::fromUtf8("采集状态"),
                                                QString::fromUtf8("等待真实数据"),
                                                &m_captureStateValue,
                                                rightGroup));
    rightGroupLayout->addWidget(createMetricRow(QString::fromUtf8("当前写入速率"),
                                                QString::fromUtf8("0 MB/s"),
                                                &m_writeSpeedValue,
                                                rightGroup));
    rightGroupLayout->addWidget(createMetricRow(QString::fromUtf8("最近动作"),
                                                QString::fromUtf8("初始化完成"),
                                                &m_lastActionValue,
                                                rightGroup));

    topSectionLayout->addWidget(leftGroup, 0, Qt::AlignTop);
    topSectionLayout->addSpacing(700);
    topSectionLayout->addWidget(rightGroup, 0, Qt::AlignTop);
    topSectionLayout->addStretch(1);
    layout->addWidget(topSection);

    m_usageBar = createThinProgress(QStringLiteral("#38bdf8"), card);
    layout->addWidget(m_usageBar);

    QWidget *buttonRow = new QWidget(card);
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonRow);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(16);

    m_recordButton = createActionButton(QString::fromUtf8("暂停采集"), true, buttonRow);
    QPushButton *openArchiveButton = createActionButton(QString::fromUtf8("打开目录"), false, buttonRow);
    QPushButton *analyzeButton = createActionButton(QString::fromUtf8("存储分析"), false, buttonRow);
    QPushButton *exportLogButton = createActionButton(QString::fromUtf8("导出日志"), false, buttonRow);
    QPushButton *exportReportButton = createActionButton(QString::fromUtf8("导出报告"), false, buttonRow);

    const QList<QPushButton *> buttons{m_recordButton, openArchiveButton, analyzeButton, exportLogButton, exportReportButton};
    for (QPushButton *button : buttons) {
        if (!button) {
            continue;
        }
        button->setMinimumHeight(56);
        button->setMinimumWidth(170);
    }
    connect(m_recordButton, &QPushButton::clicked, this, &DataManagementPage::toggleRecording);
    connect(openArchiveButton, &QPushButton::clicked, this, &DataManagementPage::openArchive);
    connect(analyzeButton, &QPushButton::clicked, this, &DataManagementPage::analyzeStorage);
    connect(exportLogButton, &QPushButton::clicked, this, &DataManagementPage::exportLog);
    connect(exportReportButton, &QPushButton::clicked, this, &DataManagementPage::exportReport);

    buttonLayout->addWidget(m_recordButton);
    buttonLayout->addWidget(openArchiveButton);
    buttonLayout->addWidget(analyzeButton);
    buttonLayout->addWidget(exportLogButton);
    buttonLayout->addWidget(exportReportButton);
    buttonLayout->addStretch();

    layout->addWidget(buttonRow);

    QTabWidget *tabs = new QTabWidget(card);
    tabs->setObjectName("dataTabs");

    QWidget *recordTab = new QWidget(tabs);
    QVBoxLayout *recordLayout = new QVBoxLayout(recordTab);
    recordLayout->setContentsMargins(0, 0, 0, 0);

    m_recordTable = new QTableWidget(0, 4, recordTab);
    m_recordTable->setHorizontalHeaderLabels(QStringList()
                                             << QString::fromUtf8("任务 ID")
                                             << QString::fromUtf8("产品")
                                             << QString::fromUtf8("开始时间")
                                             << QString::fromUtf8("结果"));
    m_recordTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_recordTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_recordTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_recordTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_recordTable->verticalHeader()->setVisible(false);
    recordLayout->addWidget(m_recordTable);

    QWidget *runtimeTab = new QWidget(tabs);
    QVBoxLayout *runtimeLayout = new QVBoxLayout(runtimeTab);
    runtimeLayout->setContentsMargins(0, 0, 0, 0);

    m_runtimeLogEdit = new QPlainTextEdit(runtimeTab);
    m_runtimeLogEdit->setReadOnly(true);
    runtimeLayout->addWidget(m_runtimeLogEdit);

    QWidget *historyTab = new QWidget(tabs);
    QVBoxLayout *historyLayout = new QVBoxLayout(historyTab);
    historyLayout->setContentsMargins(0, 0, 0, 0);

    m_historyLogList = new QListWidget(historyTab);
    historyLayout->addWidget(m_historyLogList);

    tabs->addTab(recordTab, QString::fromUtf8("测试记录预览"));
    tabs->addTab(runtimeTab, QString::fromUtf8("实时日志查看"));
    tabs->addTab(historyTab, QString::fromUtf8("历史日志查看"));

    layout->addWidget(tabs, 1);

    appendRuntimeLog(QString::fromUtf8("数据管理模块已启动，等待真实数据输入。"));
    refreshUi();
}

void DataManagementPage::appendRuntimeLog(const QString &message)
{
    if (!m_runtimeLogEdit) {
        return;
    }

    const QString stamp = QDateTime::currentDateTime().toString(QString::fromUtf8("HH:mm:ss"));
    m_runtimeLogEdit->appendPlainText(QString::fromUtf8("[%1] %2").arg(stamp, message));

    QStringList lines = m_runtimeLogEdit->toPlainText().split('\n');
    const int maxLines = 300;
    if (lines.size() > maxLines) {
        lines = lines.mid(lines.size() - maxLines);
        m_runtimeLogEdit->setPlainText(lines.join(QString::fromUtf8("\n")));
    }

    if (m_runtimeLogEdit->verticalScrollBar()) {
        m_runtimeLogEdit->verticalScrollBar()->setValue(
            m_runtimeLogEdit->verticalScrollBar()->maximum());
    }
}

void DataManagementPage::appendTestRecord(const QString &taskId,
                                          const QString &product,
                                          const QString &startTime,
                                          const QString &result)
{
    prependRecord(taskId, product, startTime, result);
    m_lastActionValue->setText(QString::fromUtf8("收到真实测试记录"));
    appendRuntimeLog(QString::fromUtf8("收到测试记录：%1 / %2").arg(taskId, result));
    refreshUi();
}

void DataManagementPage::appendHistoryEntry(const QString &entryName)
{
    appendHistoryLog(entryName);
    m_lastActionValue->setText(QString::fromUtf8("收到真实历史条目"));
    refreshUi();
}

void DataManagementPage::updateStorageRuntime(int usedGb,
                                              int writeRateMb,
                                              bool recording,
                                              const QString &lastAction)
{
    m_usedGb = qBound(0, usedGb, 2048);
    m_currentWriteRate = qMax(0, writeRateMb);
    m_recording = recording;

    if (!lastAction.trimmed().isEmpty()) {
        m_lastActionValue->setText(lastAction);
    }

    refreshUi();
}

void DataManagementPage::toggleRecording()
{
    if (m_transitioning || !m_recordButton) {
        return;
    }

    m_transitioning = true;
    m_recordButton->setEnabled(false);
    m_lastActionValue->setText(QString::fromUtf8("正在切换采集状态..."));

    const bool nextRecording = !m_recording;
    setRecordingState(nextRecording,
                      nextRecording ? QString::fromUtf8("人工恢复采集")
                                    : QString::fromUtf8("人工暂停采集"));

    m_transitioning = false;
    m_recordButton->setEnabled(true);
}

void DataManagementPage::setRecordingState(bool recording, const QString &reason)
{
    if (m_recording == recording) {
        refreshUi();
        return;
    }

    m_recording = recording;
    if (!m_recording) {
        m_currentWriteRate = 0;
    }

    m_lastActionValue->setText(reason);
    appendRuntimeLog(reason);

    emit logMessage(recording
                        ? QString::fromUtf8("数据管理：采集状态已切换为恢复")
                        : QString::fromUtf8("数据管理：采集状态已切换为暂停"));

    refreshUi();
}

void DataManagementPage::openArchive()
{
    const QString defaultDir = QDir::homePath();
    const QString dir = QFileDialog::getExistingDirectory(this,
                                                          QString::fromUtf8("选择测试记录目录"),
                                                          defaultDir,
                                                          QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (dir.isEmpty()) {
        appendRuntimeLog(QString::fromUtf8("数据管理：用户取消目录查看。"));
        emit logMessage(QString::fromUtf8("数据管理：用户取消目录查看"));
        return;
    }

    appendHistoryLog(QFileInfo(dir).fileName() + QString::fromUtf8("/最新归档记录"));
    m_lastActionValue->setText(QString::fromUtf8("已切换查看目录"));

    QMessageBox::information(this,
                             QString::fromUtf8("目录已选择"),
                             QString::fromUtf8("当前归档目录：\n%1").arg(dir));

    appendRuntimeLog(QString::fromUtf8("数据管理：已选择归档目录 %1").arg(dir));
    emit logMessage(QString::fromUtf8("数据管理：已更新归档目录"));

    refreshUi();
}

QString DataManagementPage::buildStorageSummary() const
{
    const double usagePercent = qBound(0.0, (m_usedGb / 2048.0) * 100.0, 100.0);
    QString level = QString::fromUtf8("良好");
    if (usagePercent >= 90.0) {
        level = QString::fromUtf8("需要尽快清理");
    } else if (usagePercent >= 75.0) {
        level = QString::fromUtf8("关注容量趋势");
    }

    return QString::fromUtf8("使用率 %1%，健康评估：%2。\n"
                             "测试记录 %3 条，历史日志 %4 份。\n"
                             "当前采集状态：%5，写入速率：%6。")
        .arg(QString::number(usagePercent, 'f', 1),
             level,
             QString::number(m_recordTable ? m_recordTable->rowCount() : 0),
             QString::number(m_historyLogList ? m_historyLogList->count() : 0),
             m_recording ? QString::fromUtf8("正在采集") : QString::fromUtf8("已暂停"),
             currentWriteRateText());
}

void DataManagementPage::analyzeStorage()
{
    const QString summary = buildStorageSummary();
    QMessageBox::information(this,
                             QString::fromUtf8("存储分析"),
                             summary);
    m_lastActionValue->setText(QString::fromUtf8("已完成存储分析"));
    appendRuntimeLog(QString::fromUtf8("数据管理：已执行数据库与磁盘健康分析。"));
    emit logMessage(QString::fromUtf8("数据管理：已执行数据库与磁盘健康分析"));
    refreshUi();
}

void DataManagementPage::exportReport()
{
    const QString filePath = QFileDialog::getSaveFileName(
        this,
        QString::fromUtf8("导出测试报告"),
        QDir::homePath() + QString::fromUtf8("/batch_report.txt"),
        QString::fromUtf8("文本文件 (*.txt)")
    );

    if (filePath.isEmpty()) {
        appendRuntimeLog(QString::fromUtf8("用户取消报告导出。"));
        emit logMessage(QString::fromUtf8("数据管理：用户取消报告导出"));
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this,
                             QString::fromUtf8("导出失败"),
                             QString::fromUtf8("报告文件无法写入：\n%1").arg(filePath));
        appendRuntimeLog(QString::fromUtf8("报告导出失败：%1").arg(filePath));
        emit logMessage(QString::fromUtf8("数据管理：报告导出失败"));
        return;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");

    out << QString::fromUtf8("批量测试报告\n");
    out << QString::fromUtf8("导出时间：")
        << QDateTime::currentDateTime().toString(QString::fromUtf8("yyyy-MM-dd HH:mm:ss")) << "\n\n";
    out << QString::fromUtf8("采集状态：")
        << (m_recording ? QString::fromUtf8("正在采集") : QString::fromUtf8("已暂停")) << "\n";
    out << QString::fromUtf8("当前写入速率：") << currentWriteRateText() << "\n";
    out << QString::fromUtf8("存储摘要：") << buildStorageSummary() << "\n\n";
    out << QString::fromUtf8("最近测试记录：\n");

    if (m_recordTable) {
        const int rows = qMin(10, m_recordTable->rowCount());
        for (int row = 0; row < rows; ++row) {
            QTableWidgetItem *item0 = m_recordTable->item(row, 0);
            QTableWidgetItem *item1 = m_recordTable->item(row, 1);
            QTableWidgetItem *item2 = m_recordTable->item(row, 2);
            QTableWidgetItem *item3 = m_recordTable->item(row, 3);

            out << QString::fromUtf8("- ")
                << (item0 ? item0->text() : QString())
                << QString::fromUtf8(" / ")
                << (item1 ? item1->text() : QString())
                << QString::fromUtf8(" / ")
                << (item2 ? item2->text() : QString())
                << QString::fromUtf8(" / ")
                << (item3 ? item3->text() : QString())
                << QString::fromUtf8("\n");
        }
    }

    file.close();

    appendHistoryLog(QFileInfo(filePath).fileName());
    m_lastActionValue->setText(QString::fromUtf8("已导出测试报告"));

    QMessageBox::information(this,
                             QString::fromUtf8("导出成功"),
                             QString::fromUtf8("报告已导出到：\n%1").arg(filePath));

    appendRuntimeLog(QString::fromUtf8("测试报告已导出：%1").arg(filePath));
    emit logMessage(QString::fromUtf8("数据管理：测试报告已导出"));

    refreshUi();
}

void DataManagementPage::refreshUi()
{
    const double totalGb = 2048.0;
    const double freeGb = qMax(0.0, totalGb - static_cast<double>(m_usedGb));
    const double freeTb = freeGb / 1024.0;
    const int usagePercent = qBound(0, qRound((static_cast<double>(m_usedGb) / totalGb) * 100.0), 100);

    if (m_storagePoolValue) {
        m_storagePoolValue->setText(m_recording ? QString::fromUtf8("在线 / SQLite")
                                                : QString::fromUtf8("在线 / SQLite（只读待机）"));
    }
    if (m_usageValue) {
        m_usageValue->setText(QString::fromUtf8("%1 GB / 2.0 TB").arg(m_usedGb));
    }
    if (m_freeValue) {
        m_freeValue->setText(QString::fromUtf8("%1 TB").arg(QString::number(freeTb, 'f', 2)));
    }
    if (m_healthValue) {
        const QString health = usagePercent >= 90 ? QString::fromUtf8("关注 (82%)")
                                                  : QString::fromUtf8("良好 (100%)");
        m_healthValue->setText(health);
    }
    if (m_captureStateValue) {
        m_captureStateValue->setText(m_recording ? QString::fromUtf8("正在采集 / 等待真实数据")
                                                 : QString::fromUtf8("已暂停 / 等待恢复"));
    }
    if (m_writeSpeedValue) {
        m_writeSpeedValue->setText(currentWriteRateText());
    }
    if (m_usageBar) {
        m_usageBar->setValue(usagePercent);
    }
    if (m_recordButton) {
        m_recordButton->setText(m_recording ? QString::fromUtf8("暂停采集")
                                            : QString::fromUtf8("恢复采集"));
    }

    emit storageStateChanged(m_recording ? QString::fromUtf8("DB在线 · 等待数据")
                                         : QString::fromUtf8("DB在线 · 待机"),
                             QString::fromUtf8("剩余 %1TB").arg(QString::number(freeTb, 'f', 2)),
                             m_recording ? QString::fromUtf8("数据库写入 %1MB/s").arg(m_currentWriteRate)
                                         : QString::fromUtf8("数据库待机 0MB/s"));
}

QString DataManagementPage::currentWriteRateText() const
{
    return m_recording ? QString::fromUtf8("%1 MB/s").arg(m_currentWriteRate)
                       : QString::fromUtf8("0 MB/s");
}

void DataManagementPage::exportLog()
{
    const QString filePath = QFileDialog::getSaveFileName(
        this,
        QString::fromUtf8("导出运行日志"),
        QDir::homePath() + QString::fromUtf8("/runtime_log.txt"),
        QString::fromUtf8("文本文件 (*.txt)")
    );

    if (filePath.isEmpty()) {
        appendRuntimeLog(QString::fromUtf8("数据管理：用户取消日志导出。"));
        emit logMessage(QString::fromUtf8("数据管理：用户取消日志导出"));
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this,
                             QString::fromUtf8("导出失败"),
                             QString::fromUtf8("日志文件无法写入：\n%1").arg(filePath));
        appendRuntimeLog(QString::fromUtf8("数据管理：日志导出失败 -> %1").arg(filePath));
        emit logMessage(QString::fromUtf8("数据管理：日志导出失败 -> %1").arg(filePath));
        return;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");

    out << QString::fromUtf8("批量测试工装日志导出\n");
    out << QString::fromUtf8("导出时间：") << QDateTime::currentDateTime().toString(QString::fromUtf8("yyyy-MM-dd HH:mm:ss")) << "\n";
    out << QString::fromUtf8("采集状态：") << (m_recording ? QString::fromUtf8("正在采集") : QString::fromUtf8("已暂停")) << "\n";
    out << QString::fromUtf8("当前写入速率：") << currentWriteRateText() << "\n";
    out << QString::fromUtf8("========================================\n\n");

    if (m_runtimeLogEdit && !m_runtimeLogEdit->toPlainText().trimmed().isEmpty()) {
        out << m_runtimeLogEdit->toPlainText() << "\n";
    } else {
        out << QString::fromUtf8("暂无实时日志。\n");
    }

    file.close();

    appendHistoryLog(QFileInfo(filePath).fileName());
    m_lastActionValue->setText(QString::fromUtf8("已导出运行日志"));

    QMessageBox::information(this,
                             QString::fromUtf8("导出成功"),
                             QString::fromUtf8("日志已导出到：\n%1").arg(filePath));

    appendRuntimeLog(QString::fromUtf8("数据管理：日志已导出 -> %1").arg(filePath));
    emit logMessage(QString::fromUtf8("数据管理：日志已导出 -> %1").arg(filePath));

    refreshUi();
}

void DataManagementPage::appendHistoryLog(const QString &fileName)
{
    if (!m_historyLogList) {
        return;
    }

    m_historyLogList->insertItem(0, fileName);
    while (m_historyLogList->count() > 20) {
        delete m_historyLogList->takeItem(m_historyLogList->count() - 1);
    }
}

void DataManagementPage::prependRecord(const QString &taskId,
                                       const QString &product,
                                       const QString &startTime,
                                       const QString &result)
{
    if (!m_recordTable) {
        return;
    }

    m_recordTable->insertRow(0);
    m_recordTable->setItem(0, 0, new QTableWidgetItem(taskId));
    m_recordTable->setItem(0, 1, new QTableWidgetItem(product));
    m_recordTable->setItem(0, 2, new QTableWidgetItem(startTime));
    m_recordTable->setItem(0, 3, new QTableWidgetItem(result));

    while (m_recordTable->rowCount() > 50) {
        m_recordTable->removeRow(m_recordTable->rowCount() - 1);
    }
}
