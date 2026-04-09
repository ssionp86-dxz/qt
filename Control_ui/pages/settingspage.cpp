#include "settingspage.h"

#include "detailuifactory.h"

#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHostAddress>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

using namespace DetailUiFactory;

SettingsPage::SettingsPage(QWidget *parent)
    : QWidget(parent)
    , m_summaryValue(nullptr)
    , m_dbPathEdit(nullptr)
    , m_timeIpEdit(nullptr)
    , m_timePortSpin(nullptr)
    , m_acSerialPortCombo(nullptr)
    , m_acBaudCombo(nullptr)
    , m_acParityCombo(nullptr)
    , m_acStopBitsCombo(nullptr)
    , m_acAddressSpin(nullptr)
    , m_dcSerialPortCombo(nullptr)
    , m_dcBaudCombo(nullptr)
    , m_dcParityCombo(nullptr)
    , m_dcStopBitsCombo(nullptr)
    , m_dcAddressSpin(nullptr)
    , m_videoIpEdit(nullptr)
    , m_videoPortSpin(nullptr)
    , m_appliedSummary(QString())
    , m_settingsApplied(false)
{
    setObjectName(QString::fromUtf8("settingsPage"));

    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    QFrame *card = createCardFrame(this);
    rootLayout->addWidget(card);

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(18);

    auto styleBox = [](QGroupBox *box) {
        box->setObjectName(QString::fromUtf8("settingsSectionBox"));
    };

    auto styleForm = [](QFormLayout *form) {
        form->setContentsMargins(16, 16, 16, 16);
        form->setHorizontalSpacing(14);
        form->setVerticalSpacing(14);
        form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    };

    auto styleLineEdit = [](QLineEdit *edit) {
        edit->setObjectName(QString::fromUtf8("settingsLineEdit"));
        edit->setMinimumHeight(48);
    };

    auto styleToolButton = [](QPushButton *button) {
        button->setObjectName(QString::fromUtf8("settingsBrowseButton"));
        button->setMinimumHeight(48);
        button->setMinimumWidth(110);
    };

    auto styleCombo = [](QComboBox *combo) {
        combo->setObjectName(QString::fromUtf8("settingsComboBox"));
        combo->setMinimumHeight(48);
        QListView *view = new QListView(combo);
        view->setObjectName(QString::fromUtf8("settingsComboListView"));
        view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        combo->setView(view);
    };

    auto styleSpin = [](QSpinBox *spin) {
        spin->setObjectName(QString::fromUtf8("settingsSpinBox"));
        spin->setMinimumHeight(48);
    };

    auto createSerialSection = [&](QGroupBox *box,
                                   QComboBox **portCombo,
                                   QComboBox **baudCombo,
                                   QComboBox **parityCombo,
                                   QComboBox **stopCombo,
                                   QSpinBox **addressSpin) {
        styleBox(box);
        QFormLayout *form = new QFormLayout(box);
        styleForm(form);

        *portCombo = new QComboBox(box);
        (*portCombo)->addItems(QStringList()
                               << QString::fromUtf8("COM1")
                               << QString::fromUtf8("COM2")
                               << QString::fromUtf8("COM3")
                               << QString::fromUtf8("COM4"));

        *baudCombo = new QComboBox(box);
        (*baudCombo)->addItems(QStringList()
                               << QString::fromUtf8("9600")
                               << QString::fromUtf8("19200")
                               << QString::fromUtf8("38400")
                               << QString::fromUtf8("115200"));

        *parityCombo = new QComboBox(box);
        (*parityCombo)->addItems(QStringList()
                                 << QString::fromUtf8("None")
                                 << QString::fromUtf8("Even")
                                 << QString::fromUtf8("Odd"));

        *stopCombo = new QComboBox(box);
        (*stopCombo)->addItems(QStringList()
                               << QString::fromUtf8("1")
                               << QString::fromUtf8("2"));

        *addressSpin = new QSpinBox(box);
        (*addressSpin)->setRange(1, 247);
        (*addressSpin)->setValue(1);

        styleCombo(*portCombo);
        styleCombo(*baudCombo);
        styleCombo(*parityCombo);
        styleCombo(*stopCombo);
        styleSpin(*addressSpin);

        form->addRow(QString::fromUtf8("串口号"), *portCombo);
        form->addRow(QString::fromUtf8("波特率"), *baudCombo);
        form->addRow(QString::fromUtf8("校验位"), *parityCombo);
        form->addRow(QString::fromUtf8("停止位"), *stopCombo);
        form->addRow(QString::fromUtf8("ModBus RTU 地址"), *addressSpin);
    };

    QGroupBox *dbBox = new QGroupBox(QString::fromUtf8("数据库存储位置"), card);
    styleBox(dbBox);
    QFormLayout *dbForm = new QFormLayout(dbBox);
    styleForm(dbForm);
    m_dbPathEdit = new QLineEdit(QString::fromUtf8("D:/BatchTestData"), dbBox);
    styleLineEdit(m_dbPathEdit);

    QWidget *dbPathRow = new QWidget(dbBox);
    dbPathRow->setObjectName(QString::fromUtf8("settingsDbPathRow"));
    QHBoxLayout *dbPathLayout = new QHBoxLayout(dbPathRow);
    dbPathLayout->setContentsMargins(0, 0, 0, 0);
    dbPathLayout->setSpacing(10);
    dbPathLayout->addWidget(m_dbPathEdit, 1);

    QPushButton *browseDbButton = new QPushButton(QString::fromUtf8("选择路径"), dbPathRow);
    styleToolButton(browseDbButton);
    browseDbButton->setStyleSheet(QString());
    dbPathLayout->addWidget(browseDbButton, 0);

    connect(browseDbButton, &QPushButton::clicked, this, [this]() {
        const QString currentPath = m_dbPathEdit ? m_dbPathEdit->text().trimmed() : QString();
        const QString startDir = currentPath.isEmpty() ? QString::fromUtf8("D:/") : currentPath;
        const QString dir = QFileDialog::getExistingDirectory(
            this,
            QString::fromUtf8("选择数据库存储目录"),
            startDir,
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

        if (!dir.isEmpty() && m_dbPathEdit) {
            m_dbPathEdit->setText(dir);
            refreshSummary();
        }
    });

    dbForm->addRow(QString::fromUtf8("数据库路径"), dbPathRow);

    QGroupBox *timeBox = new QGroupBox(QString::fromUtf8("授时与时差测量通信参数"), card);
    styleBox(timeBox);
    QFormLayout *timeForm = new QFormLayout(timeBox);
    styleForm(timeForm);
    m_timeIpEdit = new QLineEdit(QString::fromUtf8("192.168.10.20"), timeBox);
    m_timePortSpin = new QSpinBox(timeBox);
    m_timePortSpin->setRange(1, 65535);
    m_timePortSpin->setValue(5000);
    styleLineEdit(m_timeIpEdit);
    styleSpin(m_timePortSpin);
    timeForm->addRow(QString::fromUtf8("IP"), m_timeIpEdit);
    timeForm->addRow(QString::fromUtf8("端口"), m_timePortSpin);

    QGroupBox *videoBox = new QGroupBox(QString::fromUtf8("视频拍摄模块网络通信参数"), card);
    styleBox(videoBox);
    QFormLayout *videoForm = new QFormLayout(videoBox);
    styleForm(videoForm);
    m_videoIpEdit = new QLineEdit(QString::fromUtf8("192.168.10.60"), videoBox);
    m_videoPortSpin = new QSpinBox(videoBox);
    m_videoPortSpin->setRange(1, 65535);
    m_videoPortSpin->setValue(9000);
    styleLineEdit(m_videoIpEdit);
    styleSpin(m_videoPortSpin);
    videoForm->addRow(QString::fromUtf8("IP"), m_videoIpEdit);
    videoForm->addRow(QString::fromUtf8("端口"), m_videoPortSpin);

    QGroupBox *acBox = new QGroupBox(QString::fromUtf8("交流电源通信参数"), card);
    createSerialSection(acBox, &m_acSerialPortCombo, &m_acBaudCombo, &m_acParityCombo, &m_acStopBitsCombo, &m_acAddressSpin);
    m_acSerialPortCombo->setCurrentText(QString::fromUtf8("COM3"));
    m_acBaudCombo->setCurrentText(QString::fromUtf8("115200"));
    m_acAddressSpin->setValue(3);

    QGroupBox *dcBox = new QGroupBox(QString::fromUtf8("直流电源通信参数"), card);
    createSerialSection(dcBox, &m_dcSerialPortCombo, &m_dcBaudCombo, &m_dcParityCombo, &m_dcStopBitsCombo, &m_dcAddressSpin);
    m_dcSerialPortCombo->setCurrentText(QString::fromUtf8("COM4"));
    m_dcBaudCombo->setCurrentText(QString::fromUtf8("115200"));
    m_dcAddressSpin->setValue(4);

    QWidget *sectionRow = new QWidget(card);
    sectionRow->setObjectName(QString::fromUtf8("settingsSectionRow"));
    QHBoxLayout *sectionLayout = new QHBoxLayout(sectionRow);
    sectionLayout->setContentsMargins(0, 0, 0, 0);
    sectionLayout->setSpacing(20);

    QVBoxLayout *leftColumn = new QVBoxLayout();
    leftColumn->setSpacing(18);
    leftColumn->addWidget(dbBox);

    QWidget *netRow = new QWidget(sectionRow);
    netRow->setObjectName(QString::fromUtf8("settingsNetRow"));
    QHBoxLayout *netLayout = new QHBoxLayout(netRow);
    netLayout->setContentsMargins(0, 0, 0, 0);
    netLayout->setSpacing(18);
    netLayout->addWidget(timeBox, 1);
    netLayout->addWidget(videoBox, 1);

    leftColumn->addWidget(netRow, 1);

    QVBoxLayout *rightColumn = new QVBoxLayout();
    rightColumn->setSpacing(18);
    rightColumn->addWidget(acBox);
    rightColumn->addWidget(dcBox);

    sectionLayout->addLayout(leftColumn, 6);
    sectionLayout->addLayout(rightColumn, 5);

    layout->addWidget(sectionRow, 1);

    QWidget *buttonRow = new QWidget(card);
    buttonRow->setObjectName(QString::fromUtf8("settingsButtonRow"));
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonRow);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(16);

    QPushButton *applyButton = createActionButton(QString::fromUtf8("保存设置"), true, buttonRow);
    QPushButton *testButton = createActionButton(QString::fromUtf8("测试连接"), false, buttonRow);
    QPushButton *defaultButton = createActionButton(QString::fromUtf8("恢复默认"), false, buttonRow);

    applyButton->setObjectName(QString::fromUtf8("settingsPrimaryButton"));
    testButton->setObjectName(QString::fromUtf8("settingsSecondaryButton"));
    defaultButton->setObjectName(QString::fromUtf8("settingsSecondaryButton"));

    const QList<QPushButton *> buttons{applyButton, testButton, defaultButton};
    for (QPushButton *button : buttons) {
        button->setMinimumHeight(58);
        button->setMinimumWidth(170);
        button->setStyleSheet(QString());
    }

    connect(applyButton, &QPushButton::clicked, this, &SettingsPage::applySettings);
    connect(testButton, &QPushButton::clicked, this, &SettingsPage::testConnections);
    connect(defaultButton, &QPushButton::clicked, this, &SettingsPage::restoreDefaults);

    buttonLayout->addWidget(applyButton);
    buttonLayout->addWidget(testButton);
    buttonLayout->addWidget(defaultButton);
    buttonLayout->addStretch();

    layout->addWidget(buttonRow);

    refreshSummary();
}

bool SettingsPage::validateInputs(QString *errorMessage) const
{
    if (m_dbPathEdit->text().trimmed().isEmpty()) {
        if (errorMessage) {
            *errorMessage = QString::fromUtf8("数据库路径不能为空。");
        }
        return false;
    }

    QHostAddress timeAddr;
    if (timeAddr.setAddress(m_timeIpEdit->text().trimmed()) == false) {
        if (errorMessage) {
            *errorMessage = QString::fromUtf8("授时与时差测量 IP 地址格式不正确。");
        }
        return false;
    }

    QHostAddress videoAddr;
    if (videoAddr.setAddress(m_videoIpEdit->text().trimmed()) == false) {
        if (errorMessage) {
            *errorMessage = QString::fromUtf8("视频模块 IP 地址格式不正确。");
        }
        return false;
    }

    if (m_acSerialPortCombo->currentText() == m_dcSerialPortCombo->currentText()) {
        if (errorMessage) {
            *errorMessage = QString::fromUtf8("交流电源和直流电源不能使用同一个串口号。");
        }
        return false;
    }

    if (m_acAddressSpin->value() == m_dcAddressSpin->value()) {
        if (errorMessage) {
            *errorMessage = QString::fromUtf8("交流电源和直流电源的 ModBus RTU 地址不能相同。");
        }
        return false;
    }

    return true;
}

QString SettingsPage::buildSummaryText() const
{
    return QString::fromUtf8("DB:%1 | 授时:%2:%3 | 视频:%4:%5 | AC:%6[%7] | DC:%8[%9]")
        .arg(m_dbPathEdit->text().trimmed(),
             m_timeIpEdit->text().trimmed())
        .arg(m_timePortSpin->value())
        .arg(m_videoIpEdit->text().trimmed())
        .arg(m_videoPortSpin->value())
        .arg(m_acSerialPortCombo->currentText(),
             QString::number(m_acAddressSpin->value()))
        .arg(m_dcSerialPortCombo->currentText(),
             QString::number(m_dcAddressSpin->value()));
}

void SettingsPage::applySettings()
{
    QString errorMessage;
    if (!validateInputs(&errorMessage)) {
        QMessageBox::warning(this, QString::fromUtf8("保存失败"), errorMessage);
        emit logMessage(QString::fromUtf8("设置：保存失败 - %1").arg(errorMessage));
        return;
    }

    m_appliedSummary = buildSummaryText();
    m_settingsApplied = true;
    refreshSummary();

    QMessageBox::information(this,
                             QString::fromUtf8("保存成功"),
                             QString::fromUtf8("设置参数已保存并应用。\n\n%1").arg(m_appliedSummary));

    emit logMessage(QString::fromUtf8("设置：参数已保存并应用到运行环境 -> %1").arg(m_appliedSummary));
}

void SettingsPage::restoreDefaults()
{
    m_dbPathEdit->setText(QString::fromUtf8("D:/BatchTestData"));
    m_timeIpEdit->setText(QString::fromUtf8("192.168.10.20"));
    m_timePortSpin->setValue(5000);

    m_acSerialPortCombo->setCurrentText(QString::fromUtf8("COM3"));
    m_acBaudCombo->setCurrentText(QString::fromUtf8("115200"));
    m_acParityCombo->setCurrentText(QString::fromUtf8("None"));
    m_acStopBitsCombo->setCurrentText(QString::fromUtf8("1"));
    m_acAddressSpin->setValue(3);

    m_dcSerialPortCombo->setCurrentText(QString::fromUtf8("COM4"));
    m_dcBaudCombo->setCurrentText(QString::fromUtf8("115200"));
    m_dcParityCombo->setCurrentText(QString::fromUtf8("None"));
    m_dcStopBitsCombo->setCurrentText(QString::fromUtf8("1"));
    m_dcAddressSpin->setValue(4);

    m_videoIpEdit->setText(QString::fromUtf8("192.168.10.60"));
    m_videoPortSpin->setValue(9000);

    m_settingsApplied = false;
    m_appliedSummary.clear();
    refreshSummary();
    emit logMessage(QString::fromUtf8("设置：已恢复默认通信参数"));
}

void SettingsPage::testConnections()
{
    QString errorMessage;
    if (!validateInputs(&errorMessage)) {
        QMessageBox::warning(this,
                             QString::fromUtf8("测试失败"),
                             QString::fromUtf8("当前配置无效，无法执行测试：\n%1").arg(errorMessage));
        emit logMessage(QString::fromUtf8("设置：连接测试失败 - %1").arg(errorMessage));
        return;
    }

    const QString testSummary = buildSummaryText();

    QMessageBox::information(this,
                             QString::fromUtf8("测试连接"),
                             QString::fromUtf8("已按当前配置完成一次模拟连接检查。\n\n%1").arg(testSummary));

    emit logMessage(QString::fromUtf8("设置：已完成一次连接测试 -> %1").arg(testSummary));
}

void SettingsPage::refreshSummary()
{
    const QString summary = buildSummaryText();
    emit extraChanged(m_settingsApplied ? m_appliedSummary : summary);
}
