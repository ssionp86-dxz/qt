#include "idcpage.h"

#include "vdr_glk.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QRegExp>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

static QString nowText()
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
}

IdcPage::IdcPage(QWidget *parent)
    : QWidget(parent),
      m_serverInfo(0),
      m_savePath(0),
      m_begin(0),
      m_end(0),
      m_dir(0),
      m_typeSummary(0),
      m_progress(0),
      m_log(0),
      m_btnBrowse(0),
      m_btnQuery(0),
      m_btnSetDir(0),
      m_btnStart(0),
      m_btnStop(0),
      m_updatingDirCombo(false)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(10);

    QGroupBox *box1 = new QGroupBox(QString::fromLocal8Bit("参数信息设置"), this);
    QGridLayout *g = new QGridLayout(box1);
    g->setContentsMargins(14, 18, 14, 14);
    g->setHorizontalSpacing(14);
    g->setVerticalSpacing(10);

    m_serverInfo = new QLabel(QString::fromLocal8Bit("服务端信息："), box1);
    m_savePath = new QLineEdit(box1);
    m_savePath->setReadOnly(true);
    m_btnBrowse = new QPushButton(QString::fromLocal8Bit("参数设置"), box1);

    m_begin = new QLineEdit(box1);
    m_end = new QLineEdit(box1);
    m_end->hide();

    m_dir = new QComboBox(box1);
    m_dir->setEditable(true);
    m_dir->setInsertPolicy(QComboBox::NoInsert);

    m_btnQuery = new QPushButton(QString::fromLocal8Bit("查询目录结构"), box1);
    QPushButton *btnViewDir = new QPushButton(QString::fromLocal8Bit("查看内存目录内容"), box1);
    m_btnSetDir = new QPushButton(QString::fromLocal8Bit("设置下载时间"), box1);

    m_typeSummary = new QLabel(box1);
    m_typeSummary->setProperty("summary", true);
    m_typeSummary->setMinimumWidth(420);

    g->addWidget(m_serverInfo, 0, 0, 1, 5);
    g->addWidget(new QLabel(QString::fromLocal8Bit("下载目录"), box1), 1, 0);
    g->addWidget(m_dir, 1, 1);
    g->addWidget(new QLabel(QString::fromLocal8Bit("存储目录"), box1), 1, 2);
    g->addWidget(m_savePath, 1, 3);
    g->addWidget(m_btnBrowse, 1, 4);

    g->addWidget(new QLabel(QString::fromLocal8Bit("下载时间"), box1), 2, 0);
    g->addWidget(m_begin, 2, 1, 1, 3);
    g->addWidget(m_btnSetDir, 2, 4);

    g->addWidget(new QLabel(QString::fromLocal8Bit("数据类型"), box1), 3, 0);
    g->addWidget(m_typeSummary, 3, 1, 1, 4);

    QHBoxLayout *bottomButtons = new QHBoxLayout();
    bottomButtons->setSpacing(14);
    bottomButtons->addWidget(m_btnQuery);
    bottomButtons->addWidget(btnViewDir);
    bottomButtons->addStretch();
    g->addLayout(bottomButtons, 4, 0, 1, 5);

    g->setColumnStretch(1, 4);
    g->setColumnStretch(3, 4);

    QGroupBox *box2 = new QGroupBox(QString::fromLocal8Bit("下载设置"), this);
    QVBoxLayout *settingsLayout = new QVBoxLayout(box2);

    QHBoxLayout *typeLayout = new QHBoxLayout();
    m_types << createTypeBox("CAN", DT_CAN, box2)
            << createTypeBox(QString::fromLocal8Bit("音频"), DT_AUDIO, box2)
            << createTypeBox("LOG", DT_LOG, box2)
            << createTypeBox("CONFIG", DT_CONFIG, box2)
            << createTypeBox(QString::fromLocal8Bit("雷达"), DT_RADAR, box2)
            << createTypeBox(QString::fromLocal8Bit("视频"), DT_VIDEO, box2)
            << createTypeBox(QString::fromLocal8Bit("声呐"), DT_SONAR, box2)
            << createTypeBox(QString::fromLocal8Bit("以太网"), DT_ETHERNET, box2);

    for (int i = 0; i < m_types.size(); ++i) {
        typeLayout->addWidget(m_types.at(i));
        connect(m_types.at(i), &QCheckBox::toggled, this, &IdcPage::updateSummary);
    }
    typeLayout->addStretch();

    QHBoxLayout *actionLayout = new QHBoxLayout();
    m_progress = new QProgressBar(box2);
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    m_btnStart = new QPushButton(QString::fromLocal8Bit("开始内存下载"), box2);
    m_btnStop = new QPushButton(QString::fromLocal8Bit("停止下载"), box2);
    actionLayout->addWidget(m_progress, 1);
    actionLayout->addWidget(m_btnStart);
    actionLayout->addWidget(m_btnStop);

    settingsLayout->addLayout(typeLayout);
    settingsLayout->addLayout(actionLayout);

    QGroupBox *box3 = new QGroupBox(QString::fromLocal8Bit("运行日志"), this);
    QVBoxLayout *logLayout = new QVBoxLayout(box3);
    m_log = new QPlainTextEdit(box3);
    m_log->setReadOnly(true);
    m_log->setMinimumHeight(220);
    logLayout->addWidget(m_log, 1);

    layout->addWidget(box1);
    layout->addWidget(box2);
    layout->addWidget(box3, 1);

    connect(btnViewDir, &QPushButton::clicked, this, &IdcPage::showDirDialog);
    connect(m_dir,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this,
            [this](int) {
                if (!m_updatingDirCombo) {
                    updateTimeFromCurrentDir();
                }
            });

    updateSummary();
}

QPushButton *IdcPage::browseButton() const
{
    return m_btnBrowse;
}

QPushButton *IdcPage::queryButton() const
{
    return m_btnQuery;
}

QPushButton *IdcPage::setTimeButton() const
{
    return m_btnSetDir;
}

QPushButton *IdcPage::startButton() const
{
    return m_btnStart;
}

QPushButton *IdcPage::stopButton() const
{
    return m_btnStop;
}

void IdcPage::setServerInfo(const QString &ip, int port)
{
    m_serverInfo->setText(QString::fromLocal8Bit("服务端信息：[ %1 : %2 ]").arg(ip).arg(port));
}

void IdcPage::resetServerInfo()
{
    m_serverInfo->setText(QString::fromLocal8Bit("服务端信息："));
}

QString IdcPage::savePath() const
{
    return m_savePath ? m_savePath->text() : QString();
}

void IdcPage::setSavePath(const QString &path)
{
    if (m_savePath) {
        m_savePath->setText(path);
    }
}

QString IdcPage::currentDir() const
{
    return m_dir ? m_dir->currentText().trimmed() : QString();
}

int IdcPage::currentDirIndex() const
{
    return m_dirNumbers.indexOf(currentDir());
}

int IdcPage::dirCount() const
{
    return m_dirNumbers.size();
}

bool IdcPage::hasDirData() const
{
    return !m_dirNumbers.isEmpty() && !m_dirRanges.isEmpty();
}

QString IdcPage::beginDisplayText() const
{
    return m_begin ? m_begin->text() : QString();
}

QString IdcPage::rawTimeRangeText() const
{
    return m_end ? m_end->text() : QString();
}

void IdcPage::clearTimeRange()
{
    if (m_begin) {
        m_begin->clear();
    }
    if (m_end) {
        m_end->clear();
    }
}

void IdcPage::setTimeRange(const QDateTime &begin, const QDateTime &end)
{
    QString beginShow = begin.toString("yyyy-MM-dd hh:mm");
    QString endShow = end.toString("yyyy-MM-dd hh:mm");
    if (m_begin) {
        m_begin->setText(QString("[%1] ~ [%2]").arg(beginShow).arg(endShow));
    }
    if (m_end) {
        m_end->setText(QString("%1~%2").arg(compactTimeToken(beginShow)).arg(compactTimeToken(endShow)));
    }
}

QDateTime IdcPage::beginDateTimeForIndex(int index) const
{
    if (index < 0 || index >= m_dirBeginTokens.size()) {
        return QDateTime();
    }
    return QDateTime::fromString(formatHourTokenStart(m_dirBeginTokens.at(index)), "yyyy-MM-dd hh:mm");
}

QDateTime IdcPage::endDateTimeForIndex(int index) const
{
    if (index < 0 || index >= m_dirEndTokens.size()) {
        return QDateTime();
    }
    return QDateTime::fromString(formatHourTokenEnd(m_dirEndTokens.at(index)), "yyyy-MM-dd hh:mm");
}

long IdcPage::selectedTypeMask() const
{
    long mask = 0;
    for (int i = 0; i < m_types.size(); ++i) {
        if (m_types.at(i)->isChecked()) {
            mask |= m_types.at(i)->property("typeValue").toInt();
        }
    }
    return mask;
}

QString IdcPage::selectedTypeText() const
{
    QStringList parts;
    for (int i = 0; i < m_types.size(); ++i) {
        if (m_types.at(i)->isChecked()) {
            parts << m_types.at(i)->text();
        }
    }
    if (parts.isEmpty()) {
        return QString::fromLocal8Bit("未选择");
    }
    return QString("[") + parts.join("] [") + QString("]");
}

void IdcPage::setProgress(int value)
{
    if (m_progress) {
        m_progress->setValue(value);
    }
}

void IdcPage::appendLog(const QString &text)
{
    if (m_log) {
        m_log->appendPlainText(QString("[%1] %2").arg(nowText(), text));
    }
}

void IdcPage::populateDirComboFromRaw(const QByteArray &raw)
{
    if (!m_dir) {
        return;
    }

    QString previousDir = m_dir->currentText().trimmed();

    m_updatingDirCombo = true;
    m_dirNumbers.clear();
    m_dirRanges.clear();
    m_dirBeginTokens.clear();
    m_dirEndTokens.clear();
    m_dir->clear();

    QString text = QString::fromLocal8Bit(raw);
    QRegExp rx("\\[(\\d{10})-(\\d{10})\\](\\d+)");
    int pos = 0;
    while ((pos = rx.indexIn(text, pos)) != -1) {
        QString beginToken = rx.cap(1);
        QString endToken = rx.cap(2);
        QString dirNo = rx.cap(3);

        QString rangeText = QString("%1 ~ %2")
                .arg(formatHourTokenStart(beginToken))
                .arg(formatHourTokenEnd(endToken));

        m_dirNumbers << dirNo;
        m_dirRanges << rangeText;
        m_dirBeginTokens << beginToken;
        m_dirEndTokens << endToken;
        m_dir->addItem(dirNo);

        pos += rx.matchedLength();
    }

    if (m_dir->count() > 0) {
        int restoreIndex = previousDir.isEmpty() ? -1 : m_dir->findText(previousDir);
        if (restoreIndex >= 0) {
            m_dir->setCurrentIndex(restoreIndex);
        } else {
            m_dir->setCurrentIndex(0);
        }
    }

    m_updatingDirCombo = false;
    if (!m_dirNumbers.isEmpty()) {
        updateTimeFromCurrentDir();
    }
}

void IdcPage::showDirDialog()
{
    if (!hasDirData()) {
        QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("请先点击“查询目录结构”获取目录数据。"));
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle(QString::fromLocal8Bit("内存目录结构"));
    dlg.resize(560, 820);
    dlg.setModal(true);

    QVBoxLayout *root = new QVBoxLayout(&dlg);
    root->setContentsMargins(10, 10, 10, 10);

    QTableWidget *table = new QTableWidget(&dlg);
    table->setColumnCount(2);
    table->setRowCount(m_dirNumbers.size());

    QStringList headers;
    headers << QString::fromLocal8Bit("目录编号") << QString::fromLocal8Bit("存储时间范围");
    table->setHorizontalHeaderLabels(headers);
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    table->verticalHeader()->setVisible(false);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setShowGrid(true);

    for (int i = 0; i < m_dirNumbers.size(); ++i) {
        table->setItem(i, 0, new QTableWidgetItem(m_dirNumbers.at(i)));
        table->setItem(i, 1, new QTableWidgetItem(m_dirRanges.at(i)));
    }

    root->addWidget(table);
    dlg.exec();
}

QCheckBox *IdcPage::createTypeBox(const QString &text, int value, QWidget *parent)
{
    QCheckBox *box = new QCheckBox(text, parent);
    box->setProperty("typeValue", value);
    return box;
}

void IdcPage::updateSummary()
{
    if (m_typeSummary) {
        m_typeSummary->setText(selectedTypeText());
    }
}

QString IdcPage::formatHourTokenStart(const QString &token) const
{
    if (token.length() != 10) {
        return token;
    }
    return QString("%1-%2-%3 %4:00")
            .arg(token.mid(0, 4))
            .arg(token.mid(4, 2))
            .arg(token.mid(6, 2))
            .arg(token.mid(8, 2));
}

QString IdcPage::formatHourTokenEnd(const QString &token) const
{
    if (token.length() != 10) {
        return token;
    }
    return QString("%1-%2-%3 %4:59")
            .arg(token.mid(0, 4))
            .arg(token.mid(4, 2))
            .arg(token.mid(6, 2))
            .arg(token.mid(8, 2));
}

QString IdcPage::compactTimeToken(const QString &displayText) const
{
    QString s = displayText;
    s.remove("-");
    s.remove(":");
    s.remove(" ");
    return s;
}

void IdcPage::updateTimeFromCurrentDir()
{
    int idx = currentDirIndex();
    if (idx < 0) {
        clearTimeRange();
        return;
    }

    QString showText = QString("[%1] ~ [%2]")
            .arg(formatHourTokenStart(m_dirBeginTokens.at(idx)))
            .arg(formatHourTokenEnd(m_dirEndTokens.at(idx)));

    if (m_begin) {
        m_begin->setText(showText);
    }
    if (m_end) {
        m_end->setText(QString("%1~%2")
                       .arg(m_dirBeginTokens.at(idx) + "00")
                       .arg(m_dirEndTokens.at(idx) + "59"));
    }
}
