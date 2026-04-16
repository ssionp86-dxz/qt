#include "pdcpage.h"

#include "vdr_glk.h"

#include <QCheckBox>
#include <QDateTime>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QStringList>
#include <QSpinBox>
#include <QVBoxLayout>

static QString nowText()
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
}

PdcPage::PdcPage(QWidget *parent)
    : QWidget(parent),
      m_serverInfo(0),
      m_savePath(0),
      m_size(0),
      m_typeSummary(0),
      m_progress(0),
      m_log(0),
      m_btnBrowse(0),
      m_btnStart(0),
      m_btnStop(0)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(10);

    QGroupBox *box1 = new QGroupBox(QString::fromLocal8Bit("参数信息设置"), this);
    QGridLayout *g = new QGridLayout(box1);

    m_serverInfo = new QLabel(QString::fromLocal8Bit("服务端信息："), box1);
    m_savePath = new QLineEdit(box1);
    m_savePath->setReadOnly(true);
    m_btnBrowse = new QPushButton(QString::fromLocal8Bit("参数设置"), box1);

    m_size = new QSpinBox(box1);
    m_size->setRange(1, 1024 * 100);
    m_size->setValue(1024);
    m_size->setSuffix(" MB");

    m_typeSummary = new QLabel(box1);
    m_typeSummary->setProperty("summary", true);

    g->addWidget(m_serverInfo, 0, 0, 1, 2);
    g->addWidget(new QLabel(QString::fromLocal8Bit("存储目录"), box1), 0, 2);
    g->addWidget(m_savePath, 0, 3);
    g->addWidget(m_btnBrowse, 0, 4);
    g->addWidget(new QLabel(QString::fromLocal8Bit("下载数据量"), box1), 1, 0);
    g->addWidget(m_size, 1, 1);
    g->addWidget(new QLabel(QString::fromLocal8Bit("数据类型"), box1), 1, 2);
    g->addWidget(m_typeSummary, 1, 3, 1, 2);
    g->setColumnStretch(1, 1);
    g->setColumnStretch(3, 1);

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
        connect(m_types.at(i), &QCheckBox::toggled, this, &PdcPage::updateSummary);
    }
    typeLayout->addStretch();

    connect(m_size,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            [this](int) {
                updateSummary();
            });

    QHBoxLayout *actionLayout = new QHBoxLayout();
    m_progress = new QProgressBar(box2);
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    m_btnStart = new QPushButton(QString::fromLocal8Bit("开始外存下载"), box2);
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

    updateSummary();
}

QPushButton *PdcPage::browseButton() const
{
    return m_btnBrowse;
}

QPushButton *PdcPage::startButton() const
{
    return m_btnStart;
}

QPushButton *PdcPage::stopButton() const
{
    return m_btnStop;
}

void PdcPage::setServerInfo(const QString &ip, int port)
{
    m_serverInfo->setText(QString::fromLocal8Bit("服务端信息：[ %1 : %2 ]").arg(ip).arg(port));
}

void PdcPage::resetServerInfo()
{
    m_serverInfo->setText(QString::fromLocal8Bit("服务端信息："));
}

QString PdcPage::savePath() const
{
    return m_savePath ? m_savePath->text() : QString();
}

void PdcPage::setSavePath(const QString &path)
{
    if (m_savePath) {
        m_savePath->setText(path);
    }
}

int PdcPage::dataSizeMb() const
{
    return m_size ? m_size->value() : 0;
}

long PdcPage::selectedTypeMask() const
{
    long mask = 0;
    for (int i = 0; i < m_types.size(); ++i) {
        if (m_types.at(i)->isChecked()) {
            mask |= m_types.at(i)->property("typeValue").toInt();
        }
    }
    return mask;
}

QString PdcPage::selectedTypeText() const
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

void PdcPage::setProgress(int value)
{
    if (m_progress) {
        m_progress->setValue(value);
    }
}

void PdcPage::appendLog(const QString &text)
{
    if (m_log) {
        m_log->appendPlainText(QString("[%1] %2").arg(nowText(), text));
    }
}

QCheckBox *PdcPage::createTypeBox(const QString &text, int value, QWidget *parent)
{
    QCheckBox *box = new QCheckBox(text, parent);
    box->setProperty("typeValue", value);
    return box;
}

void PdcPage::updateSummary()
{
    if (m_typeSummary) {
        m_typeSummary->setText(selectedTypeText());
    }
}
