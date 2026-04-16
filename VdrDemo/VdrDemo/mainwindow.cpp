#include "mainwindow.h"

#include "idcpage.h"
#include "pdcpage.h"

#include <QCoreApplication>
#include <QCloseEvent>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QStringList>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <functional>

class BrowseLineEdit : public QLineEdit
{
public:
    explicit BrowseLineEdit(QWidget *parent = 0)
        : QLineEdit(parent)
    {
        setReadOnly(true);
        setCursor(Qt::PointingHandCursor);
    }

    std::function<void()> onBrowse;

protected:
    void mousePressEvent(QMouseEvent *event)
    {
        QLineEdit::mousePressEvent(event);
        if (onBrowse) {
            onBrowse();
        }
    }
};

MainWindow::MainWindow()
    : m_mode(ModeNone),
      m_timer(new QTimer(this)),
      m_tabs(0),
      m_editDll(0),
      m_editVersion(0),
      m_labelLoadState(0),
      m_idcPage(0),
      m_pdcPage(0),
      m_idcServerPort(0),
      m_pdcServerPort(0)
{
    buildUi();

    connect(m_timer, &QTimer::timeout, this, &MainWindow::updateProgress);
    m_timer->start(1000);

    m_editDll->setText(defaultDllPath());
    m_editVersion->setText("0");

    m_idcServerIp.clear();
    m_idcServerPort = 0;
    m_idcPage->resetServerInfo();
    m_idcPage->setSavePath(QString());

    m_pdcServerIp.clear();
    m_pdcServerPort = 0;
    m_pdcPage->resetServerInfo();
    m_pdcPage->setSavePath(QString());
}

MainWindow::~MainWindow()
{
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_mode != ModeNone && m_api.isLoaded()) {
        m_api.stop();
        m_mode = ModeNone;
    }

    if (m_api.isLoaded()) {
        m_api.exitLib();
    }

    QWidget::closeEvent(event);
}

void MainWindow::buildUi()
{
    setWindowTitle(QString::fromLocal8Bit("VDR数据下载程序"));
    resize(1400, 1200);
    setStyleSheet(
        "QWidget{font-family:'Microsoft YaHei';font-size:20px;background:#F6F6F6;color:#222;}"
        "QGroupBox{font-weight:bold;border:1px solid #CFCFCF;margin-top:10px;background:#FAFAFA;}"
        "QGroupBox::title{subcontrol-origin:margin;left:10px;padding:0 4px;color:#222;}"
        "QLineEdit,QSpinBox,QTextEdit,QPlainTextEdit{border:1px solid #BFBFBF;padding:5px;background:white;}"
        "QPushButton{background:#FFFFFF;border:1px solid #B7B7B7;padding:6px 14px;min-width:88px;}"
        "QPushButton:hover{background:#F0F0F0;}"
        "QPushButton:pressed{background:#E7E7E7;}"
        "QPushButton:disabled{color:#888;background:#F2F2F2;}"
        "QTabWidget::pane{border:1px solid #CFCFCF;background:#FAFAFA;top:-1px;}"
        "QTabBar::tab{padding:8px 18px;background:#EFEFEF;border:1px solid #CFCFCF;border-bottom:none;margin-right:2px;}"
        "QTabBar::tab:selected{background:#FAFAFA;font-weight:bold;}"
        "QProgressBar{border:1px solid #BFBFBF;text-align:center;background:white;}"
        "QProgressBar::chunk{background:#7CB342;}"
        "QCheckBox{spacing:6px;padding-right:8px;}"
        "QLabel[summary='true']{border:1px solid #D5D5D5;background:white;padding:4px;}"
    );

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(12, 10, 12, 12);
    root->setSpacing(8);

    m_editDll = new QLineEdit(this);
    m_editVersion = new QLineEdit(this);
    m_labelLoadState = new QLabel(this);
    m_editDll->hide();
    m_editVersion->hide();
    m_labelLoadState->hide();

    m_tabs = new QTabWidget(this);
    m_idcPage = new IdcPage(this);
    m_pdcPage = new PdcPage(this);

    m_tabs->addTab(m_idcPage, QString::fromLocal8Bit("内存下载"));
    m_tabs->addTab(m_pdcPage, QString::fromLocal8Bit("外存下载"));

    root->addWidget(m_tabs, 1);

    connect(m_idcPage->browseButton(), &QPushButton::clicked, this, &MainWindow::onBrowseIdcPath);
    connect(m_idcPage->queryButton(), &QPushButton::clicked, this, &MainWindow::onQueryIdcDirs);
    connect(m_idcPage->setTimeButton(), &QPushButton::clicked, this, &MainWindow::onSetIdcDir);
    connect(m_idcPage->startButton(), &QPushButton::clicked, this, &MainWindow::onStartIdc);
    connect(m_idcPage->stopButton(), &QPushButton::clicked, this, &MainWindow::onStop);

    connect(m_pdcPage->browseButton(), &QPushButton::clicked, this, &MainWindow::onBrowsePdcPath);
    connect(m_pdcPage->startButton(), &QPushButton::clicked, this, &MainWindow::onStartPdc);
    connect(m_pdcPage->stopButton(), &QPushButton::clicked, this, &MainWindow::onStop);
}

QString MainWindow::defaultDllPath() const
{
    return QCoreApplication::applicationDirPath() + "/vdr_glk.dll";
}

QString MainWindow::defaultSavePath() const
{
    QDir dir(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 8; ++i) {
        if (dir.exists("VdrDemo.sln")) {
            return QDir::toNativeSeparators(dir.absoluteFilePath("VdrDemoOutput"));
        }
        if (!dir.cdUp()) {
            break;
        }
    }

    return QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "/VdrDemoOutput");
}

void MainWindow::browseLineEdit(QLineEdit *edit, bool fileMode)
{
    QString value;
    if (fileMode) {
        value = QFileDialog::getOpenFileName(this, QString::fromLocal8Bit("选择 DLL"), edit->text(), "DLL (*.dll)");
    } else {
        value = QFileDialog::getExistingDirectory(this, QString::fromLocal8Bit("选择目录"), edit->text());
    }
    if (!value.isEmpty()) {
        edit->setText(QDir::toNativeSeparators(value));
    }
}

void MainWindow::appendLog(const QString &text)
{
    appendCurrentLog(text);
}

void MainWindow::appendIdcLog(const QString &text)
{
    if (m_idcPage) {
        m_idcPage->appendLog(text);
    }
}

void MainWindow::appendPdcLog(const QString &text)
{
    if (m_pdcPage) {
        m_pdcPage->appendLog(text);
    }
}

void MainWindow::appendCurrentLog(const QString &text)
{
    if (m_mode == ModeIdc) {
        appendIdcLog(text);
        return;
    }
    if (m_mode == ModePdc) {
        appendPdcLog(text);
        return;
    }

    if (m_tabs && m_tabs->currentIndex() == 1) {
        appendPdcLog(text);
    } else {
        appendIdcLog(text);
    }
}

void MainWindow::ensureApiLoaded()
{
    QString dllPath = m_editDll->text().trimmed();
    if (m_api.isLoaded()) {
        m_api.unload();
    }
    if (!m_api.load(dllPath)) {
        m_labelLoadState->setText(QString::fromLocal8Bit("加载失败"));
        QMessageBox::warning(this, QString::fromLocal8Bit("DLL"), QString::fromLocal8Bit("加载 DLL 失败：\n%1").arg(m_api.lastError()));
        appendCurrentLog(QString::fromLocal8Bit("加载 DLL 失败：%1").arg(m_api.lastError()));
        return;
    }
    m_labelLoadState->setText(QString::fromLocal8Bit("已加载"));
    applyVersionIfNeeded();
}

void MainWindow::applyVersionIfNeeded()
{
    bool ok = false;
    int ver = m_editVersion->text().trimmed().toInt(&ok);
    if (ok && ver > 0) {
        int ret = m_api.setVersion(ver);
        appendCurrentLog(QString::fromLocal8Bit("set_version(%1) => %2").arg(ver).arg(ret));
    }
}

void MainWindow::updateProgress()
{
    if (!m_api.isLoaded() || m_mode == ModeNone) {
        return;
    }

    QString file;
    int percent = 0;
    m_api.progress(file, percent);

    if (m_mode == ModeIdc && m_idcPage) {
        m_idcPage->setProgress(percent);
    } else if (m_mode == ModePdc && m_pdcPage) {
        m_pdcPage->setProgress(percent);
    }

    int sync = m_api.querySyncStatus(m_mode == ModeIdc);
    if (percent >= 100 && sync == 0) {
        if (m_mode == ModeIdc) {
            appendIdcLog(QString::fromLocal8Bit("内存数据下载完成"));
            appendIdcLog(QString::fromLocal8Bit("等待缓存数据同步到硬盘中"));
            appendIdcLog(QString::fromLocal8Bit("缓存数据写入硬盘操作已完成"));
        } else if (m_mode == ModePdc) {
            appendPdcLog(QString::fromLocal8Bit("外存数据下载完成"));
            appendPdcLog(QString::fromLocal8Bit("等待缓存数据同步到硬盘中"));
            appendPdcLog(QString::fromLocal8Bit("缓存数据写入硬盘操作已完成"));
        }
        m_mode = ModeNone;
    }
}

void MainWindow::onBrowseDll()
{
    browseLineEdit(m_editDll, true);
}

void MainWindow::onBrowseIdcPath()
{
    QDialog dlg(this);
    dlg.setWindowTitle(QString::fromLocal8Bit("内存下载参数设置"));
    dlg.setModal(true);
    dlg.resize(430, 180);

    QVBoxLayout *root = new QVBoxLayout(&dlg);
    QFormLayout *form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    QLineEdit *editIp = new QLineEdit(&dlg);
    editIp->setText(m_idcServerIp.isEmpty() ? QString("10.0.0.2") : m_idcServerIp);

    QLineEdit *editPort = new QLineEdit(&dlg);
    editPort->setText(QString::number(m_idcServerPort > 0 ? m_idcServerPort : 6000));

    BrowseLineEdit *editSavePath = new BrowseLineEdit(&dlg);
    QString currentPath = m_idcPage ? m_idcPage->savePath().trimmed() : QString();
    editSavePath->setText(currentPath.isEmpty()
                          ? QDir::toNativeSeparators(defaultSavePath() + "/idc")
                          : currentPath);
    editSavePath->onBrowse = [&]() {
        QString dir = QFileDialog::getExistingDirectory(&dlg,
                                                        QString::fromLocal8Bit("选择内存下载存储目录"),
                                                        editSavePath->text());
        if (!dir.isEmpty()) {
            editSavePath->setText(QDir::toNativeSeparators(dir));
        }
    };

    form->addRow(QString::fromLocal8Bit("服务器IP"), editIp);
    form->addRow(QString::fromLocal8Bit("服务器端口"), editPort);
    form->addRow(QString::fromLocal8Bit("存储目录"), editSavePath);

    QPushButton *btnApply = new QPushButton(QString::fromLocal8Bit("设置参数"), &dlg);
    btnApply->setDefault(true);

    root->addLayout(form);
    root->addWidget(btnApply, 0, Qt::AlignHCenter);

    connect(btnApply, &QPushButton::clicked, &dlg, [&]() {
        bool ok = false;
        int port = editPort->text().trimmed().toInt(&ok);
        if (editIp->text().trimmed().isEmpty()) {
            QMessageBox::warning(&dlg, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("请输入服务器IP。"));
            return;
        }
        if (!ok || port <= 0) {
            QMessageBox::warning(&dlg, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("请输入正确的服务器端口。"));
            return;
        }
        if (editSavePath->text().trimmed().isEmpty()) {
            QMessageBox::warning(&dlg, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("请选择存储目录。"));
            return;
        }

        applyIdcParameters(editIp->text().trimmed(), port, editSavePath->text().trimmed());
        dlg.accept();
    });

    dlg.exec();
}

void MainWindow::onBrowsePdcPath()
{
    QDialog dlg(this);
    dlg.setWindowTitle(QString::fromLocal8Bit("外存下载参数设置"));
    dlg.setModal(true);
    dlg.resize(430, 180);

    QVBoxLayout *root = new QVBoxLayout(&dlg);
    QFormLayout *form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    QLineEdit *editIp = new QLineEdit(&dlg);
    editIp->setText(m_pdcServerIp.isEmpty() ? QString("10.0.0.3") : m_pdcServerIp);

    QLineEdit *editPort = new QLineEdit(&dlg);
    editPort->setText(QString::number(m_pdcServerPort > 0 ? m_pdcServerPort : 5000));

    BrowseLineEdit *editSavePath = new BrowseLineEdit(&dlg);
    QString currentPath = m_pdcPage ? m_pdcPage->savePath().trimmed() : QString();
    editSavePath->setText(currentPath.isEmpty()
                          ? QDir::toNativeSeparators(defaultSavePath() + "/pdc")
                          : currentPath);
    editSavePath->onBrowse = [&]() {
        QString dir = QFileDialog::getExistingDirectory(&dlg,
                                                        QString::fromLocal8Bit("选择外存下载存储目录"),
                                                        editSavePath->text());
        if (!dir.isEmpty()) {
            editSavePath->setText(QDir::toNativeSeparators(dir));
        }
    };

    form->addRow(QString::fromLocal8Bit("服务器IP"), editIp);
    form->addRow(QString::fromLocal8Bit("服务器端口"), editPort);
    form->addRow(QString::fromLocal8Bit("存储目录"), editSavePath);

    QPushButton *btnApply = new QPushButton(QString::fromLocal8Bit("设置参数"), &dlg);
    btnApply->setDefault(true);

    root->addLayout(form);
    root->addWidget(btnApply, 0, Qt::AlignHCenter);

    connect(btnApply, &QPushButton::clicked, &dlg, [&]() {
        bool ok = false;
        int port = editPort->text().trimmed().toInt(&ok);
        if (editIp->text().trimmed().isEmpty()) {
            QMessageBox::warning(&dlg, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("请输入服务器IP。"));
            return;
        }
        if (!ok || port <= 0) {
            QMessageBox::warning(&dlg, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("请输入正确的服务器端口。"));
            return;
        }
        if (editSavePath->text().trimmed().isEmpty()) {
            QMessageBox::warning(&dlg, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("请选择存储目录。"));
            return;
        }

        applyPdcParameters(editIp->text().trimmed(), port, editSavePath->text().trimmed());
        dlg.accept();
    });

    dlg.exec();
}

void MainWindow::applyIdcParameters(const QString &ip, int port, const QString &savePath)
{
    m_idcServerIp = ip;
    m_idcServerPort = port;
    if (m_idcPage) {
        m_idcPage->setServerInfo(m_idcServerIp, m_idcServerPort);
        m_idcPage->setSavePath(QDir::toNativeSeparators(savePath));
    }
    appendIdcLog(QString::fromLocal8Bit("设置数据存储路径:%1").arg(m_idcPage ? m_idcPage->savePath() : QString()));
    appendIdcLog(QString::fromLocal8Bit("设置服务端信息:[ %1 : %2 ]").arg(m_idcServerIp).arg(m_idcServerPort));
}

void MainWindow::applyPdcParameters(const QString &ip, int port, const QString &savePath)
{
    m_pdcServerIp = ip;
    m_pdcServerPort = port;
    if (m_pdcPage) {
        m_pdcPage->setServerInfo(m_pdcServerIp, m_pdcServerPort);
        m_pdcPage->setSavePath(QDir::toNativeSeparators(savePath));
    }
    appendPdcLog(QString::fromLocal8Bit("设置数据存储路径:%1").arg(m_pdcPage ? m_pdcPage->savePath() : QString()));
    appendPdcLog(QString::fromLocal8Bit("设置服务端信息:[ %1 : %2 ]").arg(m_pdcServerIp).arg(m_pdcServerPort));
}

void MainWindow::onQueryIdcDirs()
{
    ensureApiLoaded();
    if (!m_api.isLoaded()) {
        return;
    }

    int mode = 0;
    int count = 0;
    QByteArray raw;
    int ret = m_api.queryIdcDirs(mode, count, raw);

    if (ret != 0) {
        appendIdcLog(QString::fromLocal8Bit("查询目录结构失败，ret=%1").arg(ret));
        QMessageBox::warning(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("查询目录结构失败，ret=%1").arg(ret));
        return;
    }

    if (m_idcPage) {
        m_idcPage->populateDirComboFromRaw(raw);
    }

    if (!m_idcPage || !m_idcPage->hasDirData()) {
        appendIdcLog(QString::fromLocal8Bit("查询目录结构完成，但没有解析到目录数据。"));
        QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("查询完成，但没有解析到目录数据。"));
        return;
    }

    appendIdcLog(QString::fromLocal8Bit("查询目录结构成功，共解析到 %1 条目录记录。当前目录：%2")
                 .arg(m_idcPage->dirCount())
                 .arg(m_idcPage->currentDir()));
    m_idcPage->showDirDialog();
}

void MainWindow::onSetIdcDir()
{
    if (!m_idcPage || !m_idcPage->hasDirData()) {
        QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("请先点击“查询目录结构”。"));
        return;
    }

    QString currentDir = m_idcPage->currentDir();
    int idx = m_idcPage->currentDirIndex();
    if (idx < 0) {
        QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("请先选择有效的下载目录。"));
        return;
    }

    ensureApiLoaded();
    if (!m_api.isLoaded()) {
        return;
    }

    int ret = m_api.setIdcDir(currentDir);
    if (ret != 0) {
        appendIdcLog(QString::fromLocal8Bit("设置下载目录('%1') 失败 => %2").arg(currentDir).arg(ret));
        QMessageBox::warning(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("设置下载目录失败，ret=%1").arg(ret));
        return;
    }

    appendIdcLog(QString::fromLocal8Bit("设置下载目录[%1]成功!").arg(currentDir));

    QDialog dlg(this);
    dlg.setWindowTitle("set time range");
    dlg.setModal(true);
    dlg.resize(360, 170);

    QVBoxLayout *root = new QVBoxLayout(&dlg);
    QFormLayout *form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    QDateTime beginDt = m_idcPage->beginDateTimeForIndex(idx);
    QDateTime endDt = m_idcPage->endDateTimeForIndex(idx);

    QDateTimeEdit *beginEdit = new QDateTimeEdit(beginDt, &dlg);
    QDateTimeEdit *endEdit = new QDateTimeEdit(endDt, &dlg);
    beginEdit->setDisplayFormat(QString::fromLocal8Bit("yyyy年MM月dd日 hh:mm"));
    endEdit->setDisplayFormat(QString::fromLocal8Bit("yyyy年MM月dd日 hh:mm"));
    beginEdit->setCalendarPopup(true);
    endEdit->setCalendarPopup(true);

    form->addRow(QString::fromLocal8Bit("开始时间"), beginEdit);
    form->addRow(QString::fromLocal8Bit("结束时间"), endEdit);

    QPushButton *btnOk = new QPushButton(QString::fromLocal8Bit("确认"), &dlg);
    btnOk->setDefault(true);

    root->addLayout(form);
    root->addWidget(btnOk, 0, Qt::AlignHCenter);

    connect(btnOk, &QPushButton::clicked, &dlg, [&]() {
        if (beginEdit->dateTime() > endEdit->dateTime()) {
            QMessageBox::warning(&dlg, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("开始时间不能晚于结束时间。"));
            return;
        }

        m_idcPage->setTimeRange(beginEdit->dateTime(), endEdit->dateTime());
        appendIdcLog(QString::fromLocal8Bit("设置内存数据下载时间范围:[%1] ~ [%2]")
                     .arg(beginEdit->dateTime().toString("yyyy-MM-dd hh:mm"))
                     .arg(endEdit->dateTime().toString("yyyy-MM-dd hh:mm")));
        dlg.accept();
    });

    dlg.exec();
}

void MainWindow::onStartIdc()
{
    ensureApiLoaded();
    if (!m_api.isLoaded() || !m_idcPage) {
        return;
    }

    QString currentDir = m_idcPage->currentDir();
    if (currentDir.isEmpty()) {
        QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("请先查询并选择下载目录。"));
        return;
    }
    if (m_idcPage->beginDisplayText().trimmed().isEmpty() || m_idcPage->rawTimeRangeText().trimmed().isEmpty()) {
        QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("请先设置下载时间。"));
        return;
    }

    long type = m_idcPage->selectedTypeMask();
    if (type == 0) {
        QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("请至少选择一种数据类型。"));
        return;
    }

    appendIdcLog(QString::fromLocal8Bit("设置内存数据下载类型:%1").arg(m_idcPage->selectedTypeText()));
    appendIdcLog(QString::fromLocal8Bit("数据存储目录:%1").arg(m_idcPage->savePath()));
    appendIdcLog(QString::fromLocal8Bit("选择下载目录:%1").arg(currentDir));
    appendIdcLog(QString::fromLocal8Bit("下载时间:%1").arg(m_idcPage->rawTimeRangeText()));

    QDir().mkpath(m_idcPage->savePath());
    m_idcPage->setProgress(0);

    QStringList timeParts = m_idcPage->rawTimeRangeText().split("~");
    QString beginRaw = timeParts.size() > 0 ? timeParts.at(0).trimmed() : QString();
    QString endRaw = timeParts.size() > 1 ? timeParts.at(1).trimmed() : QString();

    int ret = m_api.startIdc(m_idcPage->savePath(), beginRaw, endRaw, type);
    m_mode = (ret == 0) ? ModeIdc : ModeNone;
    appendIdcLog(QString::fromLocal8Bit("开始下载内存文件..."));
    appendIdcLog(QString::fromLocal8Bit("vdrdown_idc(path='%1', begin='%2', end='%3', type=%4) => %5")
                 .arg(m_idcPage->savePath())
                 .arg(beginRaw)
                 .arg(endRaw)
                 .arg(type)
                 .arg(ret));
}

void MainWindow::onStartPdc()
{
    ensureApiLoaded();
    if (!m_api.isLoaded() || !m_pdcPage) {
        return;
    }

    if (m_pdcPage->savePath().trimmed().isEmpty()) {
        QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("请先设置外存下载存储目录。"));
        return;
    }

    long type = m_pdcPage->selectedTypeMask();
    if (type == 0) {
        QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("请至少选择一种数据类型。"));
        return;
    }

    appendPdcLog(QString::fromLocal8Bit("设置外存数据下载类型:%1").arg(m_pdcPage->selectedTypeText()));
    appendPdcLog(QString::fromLocal8Bit("数据存储目录:%1").arg(m_pdcPage->savePath()));
    appendPdcLog(QString::fromLocal8Bit("下载数据量:%1 MB").arg(m_pdcPage->dataSizeMb()));

    QDir().mkpath(m_pdcPage->savePath());
    m_pdcPage->setProgress(0);

    int ret = m_api.startPdc(m_pdcPage->savePath(), m_pdcPage->dataSizeMb(), type);
    m_mode = (ret == 0) ? ModePdc : ModeNone;
    if (ret == 0) {
        appendPdcLog(QString::fromLocal8Bit("开始下载外存文件..."));
    } else {
        appendPdcLog(QString::fromLocal8Bit("开始外存下载失败，ret=%1").arg(ret));
    }
    appendPdcLog(QString::fromLocal8Bit("vdrdown_pdc(path='%1', sizeMB=%2, type=%3) => %4")
                 .arg(m_pdcPage->savePath())
                 .arg(m_pdcPage->dataSizeMb())
                 .arg(type)
                 .arg(ret));
}

void MainWindow::onStop()
{
    QObject *who = sender();

    if (m_mode == ModeNone) {
        if (m_pdcPage && who == m_pdcPage->stopButton()) {
            appendPdcLog(QString::fromLocal8Bit("当前没有正在进行的外存下载任务。"));
        } else {
            appendIdcLog(QString::fromLocal8Bit("当前没有正在进行的内存下载任务。"));
        }
        return;
    }

    if (!m_api.isLoaded()) {
        if (m_mode == ModePdc) {
            appendPdcLog(QString::fromLocal8Bit("下载任务状态异常：DLL 未加载，停止操作未执行。"));
        } else {
            appendIdcLog(QString::fromLocal8Bit("下载任务状态异常：DLL 未加载，停止操作未执行。"));
        }
        m_mode = ModeNone;
        return;
    }

    int currentMode = m_mode;
    int ret = m_api.stop();
    if (ret == 0) {
        if (currentMode == ModeIdc) {
            appendIdcLog(QString::fromLocal8Bit("内存下载停止成功。"));
        } else if (currentMode == ModePdc) {
            appendPdcLog(QString::fromLocal8Bit("外存下载停止成功。"));
        }
    } else {
        if (currentMode == ModeIdc) {
            appendIdcLog(QString::fromLocal8Bit("停止内存下载失败，ret=%1").arg(ret));
        } else if (currentMode == ModePdc) {
            appendPdcLog(QString::fromLocal8Bit("停止外存下载失败，ret=%1").arg(ret));
        }
    }

    m_mode = ModeNone;
}
