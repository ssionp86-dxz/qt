#include "powermanagementpage.h"

#include "detailuifactory.h"

#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSlider>
#include <QVBoxLayout>
#include <QtMath>
#include <QDateTime>
#include <QMessageBox>
#include <QIcon>
#include <QSize>
using namespace DetailUiFactory;

PowerManagementPage::PowerManagementPage(QWidget *parent)
    : QWidget(parent)
    , m_poweredOn(false)
    , m_dcOutputValue(nullptr)
    , m_acOutputValue(nullptr)
    , m_freqValue(nullptr)
    , m_powerButton(nullptr)
    , m_loadBar(nullptr)
    , m_dcSpin(nullptr)
    , m_acSpin(nullptr)
    , m_freqSpin(nullptr)
{
    setObjectName(QStringLiteral("powerManagementPage"));

    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    QFrame *card = createCardFrame(this);
    rootLayout->addWidget(card);

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(20);

    QWidget *statusRow = new QWidget(card);
    QHBoxLayout *statusLayout = new QHBoxLayout(statusRow);
    statusLayout->setContentsMargins(0, 0, 0, 0);
    statusLayout->setSpacing(14);

    m_powerButton = createActionButton(QString(), true, statusRow);
    m_powerButton->setFixedSize(72, 72);
    m_powerButton->setIconSize(QSize(40, 40));
    m_powerButton->setToolTip(QString::fromUtf8("开启或关闭输出"));
    m_powerButton->setText(QString());
    m_powerButton->setIcon(QIcon(QStringLiteral(":/image/resources/power.png")));
    connect(m_powerButton, &QPushButton::clicked, this, &PowerManagementPage::togglePower);

    statusLayout->addWidget(m_powerButton, 0, Qt::AlignLeft | Qt::AlignTop);
    statusLayout->addStretch();

    layout->addWidget(statusRow);

    QWidget *metricsRow = new QWidget(card);
    QHBoxLayout *metricsLayout = new QHBoxLayout(metricsRow);
    metricsLayout->setContentsMargins(0, 0, 0, 0);
    metricsLayout->setSpacing(36);

    QWidget *dcInfoRow = createInfoRow(QString::fromUtf8("直流输出结果"), QString::fromUtf8("24.00 V"), &m_dcOutputValue, card);
    QWidget *acInfoRow = createInfoRow(QString::fromUtf8("交流输出结果"), QString::fromUtf8("220.0 V"), &m_acOutputValue, card);
    QWidget *freqInfoRow = createInfoRow(QString::fromUtf8("交流输出频率"), QString::fromUtf8("50.00 Hz"), &m_freqValue, card);

    auto enlargeInfoRow = [](QWidget *row, QLabel *valueLabel) {
        if (!row) {
            return;
        }
        row->setMinimumHeight(72);
        const auto labels = row->findChildren<QLabel *>();
        for (QLabel *label : labels) {
            QFont font = label->font();
            if (label == valueLabel) {
                label->setObjectName(QStringLiteral("powerMetricValue"));
                font.setPointSize(qMax(font.pointSize(), 15));
                font.setBold(true);
                label->setFont(font);
                label->setMinimumHeight(42);
                label->setMinimumWidth(112);
            } else {
                label->setObjectName(QStringLiteral("powerMetricLabel"));
                font.setPointSize(qMax(font.pointSize(), 17));
                font.setBold(false);
                label->setFont(font);
            }
        }
    };

    enlargeInfoRow(dcInfoRow, m_dcOutputValue);
    enlargeInfoRow(acInfoRow, m_acOutputValue);
    enlargeInfoRow(freqInfoRow, m_freqValue);

    QVBoxLayout *metricsLeftLayout = new QVBoxLayout;
    metricsLeftLayout->setContentsMargins(0, 0, 0, 0);
    metricsLeftLayout->setSpacing(18);
    metricsLeftLayout->addWidget(dcInfoRow);
    metricsLeftLayout->addWidget(acInfoRow);
    metricsLeftLayout->addWidget(freqInfoRow);
    metricsLeftLayout->addStretch();

    metricsLayout->addLayout(metricsLeftLayout, 1);

    layout->addWidget(metricsRow);

    m_loadBar = createThinProgress(QStringLiteral("#f97316"), card);
    layout->addWidget(m_loadBar);

    QWidget *configRow = new QWidget(card);
    configRow->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QHBoxLayout *configLayout = new QHBoxLayout(configRow);
    configLayout->setContentsMargins(0, 0, 0, 0);
    configLayout->setSpacing(16);

    // ========== 直流控制 ==========
    QGroupBox *dcBox = new QGroupBox(QString::fromUtf8("直流电源输出控制"), configRow);
    dcBox->setProperty("compactTitle", true);
    dcBox->setMinimumHeight(150);
    dcBox->setObjectName("sectionBox");
    QFormLayout *dcForm = new QFormLayout(dcBox);
    dcForm->setLabelAlignment(Qt::AlignLeft);

    m_dcSpin = new QDoubleSpinBox(dcBox);
    m_dcSpin->setRange(16.0, 32.0);
    m_dcSpin->setDecimals(2);
    m_dcSpin->setSingleStep(0.01);
    m_dcSpin->setSuffix(QString::fromUtf8(" V"));
    m_dcSpin->setValue(24.00);

    QSlider *dcSlider = new QSlider(Qt::Horizontal, dcBox);
    dcSlider->setRange(1600, 3200);
    dcSlider->setSingleStep(1);
    dcSlider->setPageStep(25);
    dcSlider->setValue(2400);

    dcForm->addRow(createSliderControlRow(QString::fromUtf8("目标电压"),
                                          m_dcSpin,
                                          dcSlider,
                                          100,
                                          dcBox));

    // ========== 交流控制 ==========
    QGroupBox *acBox = new QGroupBox(QString::fromUtf8("交流电源输出控制"), configRow);
    acBox->setProperty("compactTitle", true);
    acBox->setMinimumHeight(150);
    acBox->setObjectName("sectionBox");
    QFormLayout *acForm = new QFormLayout(acBox);
    acForm->setLabelAlignment(Qt::AlignLeft);

    m_acSpin = new QDoubleSpinBox(acBox);
    m_acSpin->setRange(150.0, 286.0);
    m_acSpin->setDecimals(1);
    m_acSpin->setSingleStep(0.1);
    m_acSpin->setSuffix(QString::fromUtf8(" V"));
    m_acSpin->setValue(220.0);

    QSlider *acSlider = new QSlider(Qt::Horizontal, acBox);
    acSlider->setRange(1500, 2860);
    acSlider->setSingleStep(1);
    acSlider->setPageStep(10);
    acSlider->setValue(2200);

    m_freqSpin = new QDoubleSpinBox(acBox);
    m_freqSpin->setRange(45.0, 55.0);
    m_freqSpin->setDecimals(2);
    m_freqSpin->setSingleStep(0.01);
    m_freqSpin->setSuffix(QString::fromUtf8(" Hz"));
    m_freqSpin->setValue(50.00);

    QSlider *freqSlider = new QSlider(Qt::Horizontal, acBox);
    freqSlider->setRange(4500, 5500);
    freqSlider->setSingleStep(1);
    freqSlider->setPageStep(20);
    freqSlider->setValue(5000);

    acForm->addRow(createSliderControlRow(QString::fromUtf8("目标电压"),
                                          m_acSpin,
                                          acSlider,
                                          10,
                                          acBox));

    acForm->addRow(createSliderControlRow(QString::fromUtf8("目标频率"),
                                          m_freqSpin,
                                          freqSlider,
                                          100,
                                          acBox));

    configLayout->addWidget(dcBox, 1);
    configLayout->addWidget(acBox, 1);

    layout->addWidget(configRow, 1);

    connect(m_dcSpin,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this,
            [this](double) {
                emitPowerSummary();
            });

    connect(m_acSpin,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this,
            [this](double) {
                emitPowerSummary();
            });

    connect(m_freqSpin,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this,
            [this](double) {
                emitPowerSummary();
            });
    QWidget *buttonRow = new QWidget(card);
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonRow);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(18);

    QPushButton *applyButton = createActionButton(QString::fromUtf8("应用输出设置"), true, buttonRow);
    QPushButton *safePresetButton = createActionButton(QString::fromUtf8("安全预置"), false, buttonRow);
    QPushButton *resetButton = createActionButton(QString::fromUtf8("恢复默认"), false, buttonRow);

    applyButton->setMinimumHeight(58);
    safePresetButton->setMinimumHeight(58);
    resetButton->setMinimumHeight(58);
    applyButton->setMinimumWidth(186);
    safePresetButton->setMinimumWidth(162);
    resetButton->setMinimumWidth(162);
    QFont bottomButtonFont = applyButton->font();
    bottomButtonFont.setPointSize(bottomButtonFont.pointSize() + 5);
    applyButton->setFont(bottomButtonFont);
    safePresetButton->setFont(bottomButtonFont);
    resetButton->setFont(bottomButtonFont);
    connect(applyButton, &QPushButton::clicked, this, &PowerManagementPage::applyOutputSettings);
    connect(safePresetButton, &QPushButton::clicked, this, &PowerManagementPage::applySafePreset);
    connect(resetButton, &QPushButton::clicked, this, &PowerManagementPage::resetValues);

    buttonLayout->addWidget(applyButton);
    buttonLayout->addWidget(safePresetButton);
    buttonLayout->addWidget(resetButton);
    buttonLayout->addStretch();

    layout->addWidget(buttonRow);

    refreshUi();
}

QWidget *PowerManagementPage::createSliderControlRow(const QString &labelText,
                                                     QDoubleSpinBox *spinBox,
                                                     QSlider *slider,
                                                     int sliderFactor,
                                                     QWidget *parent)
{
    QWidget *row = new QWidget(parent);
    QHBoxLayout *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 8, 0, 8);
    layout->setSpacing(20);

    QLabel *label = new QLabel(labelText, row);
    label->setMinimumWidth(112);
    QFont labelFont = label->font();
    labelFont.setPointSize(labelFont.pointSize() + 5);
    label->setFont(labelFont);

    spinBox->setMinimumWidth(280);
    spinBox->setMinimumHeight(60);


    slider->setObjectName(QStringLiteral("controlSlider"));
    slider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    slider->setMinimumHeight(40);

    layout->addWidget(label);
    layout->addWidget(spinBox, 0);
    layout->addWidget(slider, 1);

    connect(slider, &QSlider::valueChanged, this, [spinBox, sliderFactor](int value) {
        const QSignalBlocker blocker(spinBox);
        spinBox->setValue(static_cast<double>(value) / static_cast<double>(sliderFactor));
    });

    connect(spinBox,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this,
            [slider, sliderFactor](double value) {
                const QSignalBlocker blocker(slider);
                slider->setValue(qRound(value * static_cast<double>(sliderFactor)));
            });

    return row;
}

void PowerManagementPage::togglePower()
{
    m_poweredOn = !m_poweredOn;
    refreshUi();
    emit powerStateChanged(m_poweredOn);
    emit logMessage(m_poweredOn
                        ? QString::fromUtf8("电源管理：已开启 AC/DC 输出")
                        : QString::fromUtf8("电源管理：已关闭 AC/DC 输出"));
}

void PowerManagementPage::resetValues()
{
    m_dcSpin->setValue(24.00);
    m_acSpin->setValue(220.0);
    m_freqSpin->setValue(50.00);
    m_poweredOn = true;
    refreshUi();
    emit powerStateChanged(m_poweredOn);
    emit logMessage(QString::fromUtf8("电源管理：已恢复默认输出设置"));
}

void PowerManagementPage::applySafePreset()
{
    m_dcSpin->setValue(22.50);
    m_acSpin->setValue(210.0);
    m_freqSpin->setValue(49.80);
    refreshUi();
    emit logMessage(QString::fromUtf8("电源管理：已切换到安全预置档"));
}

void PowerManagementPage::applyOutputSettings()
{
    if (!m_dcSpin || !m_acSpin || !m_freqSpin) {
        return;
    }

    // 1. 先做基础校验
    const double dcValue = m_dcSpin->value();
    const double acValue = m_acSpin->value();
    const double freqValue = m_freqSpin->value();

    if (dcValue < 16.0 || dcValue > 32.0) {
        QMessageBox::warning(this,
                             QString::fromUtf8("参数错误"),
                             QString::fromUtf8("直流目标电压超出允许范围：16.00V ~ 32.00V"));
        return;
    }

    if (acValue < 150.0 || acValue > 286.0) {
        QMessageBox::warning(this,
                             QString::fromUtf8("参数错误"),
                             QString::fromUtf8("交流目标电压超出允许范围：150.0V ~ 286.0V"));
        return;
    }

    if (freqValue < 45.0 || freqValue > 55.0) {
        QMessageBox::warning(this,
                             QString::fromUtf8("参数错误"),
                             QString::fromUtf8("交流目标频率超出允许范围：45.00Hz ~ 55.00Hz"));
        return;
    }

    // 2. 电源没开启，提示
    if (!m_poweredOn) {
        QMessageBox::warning(this,
                             QString::fromUtf8("无法应用"),
                             QString::fromUtf8("当前输出处于关闭状态，请先开启输出后再应用设置。"));
        return;
    }

    // 3. 显示结果
    if (m_dcOutputValue) {
        m_dcOutputValue->setText(QString::fromUtf8("%1 V")
                                 .arg(QString::number(dcValue, 'f', 2)));
    }

    if (m_acOutputValue) {
        m_acOutputValue->setText(QString::fromUtf8("%1 V")
                                 .arg(QString::number(acValue, 'f', 1)));
    }

    if (m_freqValue) {
        m_freqValue->setText(QString::fromUtf8("%1 Hz")
                             .arg(QString::number(freqValue, 'f', 2)));
    }

    // 4. 更新界面状态
    if (m_loadBar) {
        m_loadBar->setValue(72);
    }

    // 5. 记录日志
    const QString logText = QString::fromUtf8("电源管理：输出设置已应用，DC %1V / AC %2V / %3Hz")
        .arg(QString::number(dcValue, 'f', 2),
             QString::number(acValue, 'f', 1),
             QString::number(freqValue, 'f', 2));

    emit logMessage(logText);

    // 6. 生成一条真实操作记录，送到数据管理页
    const QString taskId = QString::fromUtf8("PWR-%1")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMddHHmmss")));

    const QString startTime = QDateTime::currentDateTime()
        .toString(QString::fromUtf8("yyyy-MM-dd HH:mm:ss"));

    const QString result = QString::fromUtf8("DC %1V / AC %2V / %3Hz 已应用")
        .arg(QString::number(dcValue, 'f', 2),
             QString::number(acValue, 'f', 1),
             QString::number(freqValue, 'f', 2));

    emit outputSettingsApplied(taskId,
                               QString::fromUtf8("电源输出设置"),
                               startTime,
                               result);

    // 7. 反馈
    QMessageBox::information(this,
                             QString::fromUtf8("应用成功"),
                             QString::fromUtf8("输出设置已生效。\n\n直流：%1 V\n交流：%2 V\n频率：%3 Hz")
                                 .arg(QString::number(dcValue, 'f', 2),
                                      QString::number(acValue, 'f', 1),
                                      QString::number(freqValue, 'f', 2)));
}

void PowerManagementPage::refreshUi()
{
    if (m_poweredOn) {
        m_dcOutputValue->setText(QString::fromUtf8("%1 V").arg(QString::number(m_dcSpin->value(), 'f', 2)));
        m_acOutputValue->setText(QString::fromUtf8("%1 V").arg(QString::number(m_acSpin->value(), 'f', 1)));
        m_freqValue->setText(QString::fromUtf8("%1 Hz").arg(QString::number(m_freqSpin->value(), 'f', 2)));
        if (m_powerButton) {
            m_powerButton->setText(QString());
            m_powerButton->setIcon(QIcon(QStringLiteral(":/image/resources/power.png")));
        }
        m_loadBar->setValue(72);
        updateStateBadge(true);
    } else {
        m_dcOutputValue->setText(QString::fromUtf8("0.00 V"));
        m_acOutputValue->setText(QString::fromUtf8("0.0 V"));
        m_freqValue->setText(QString::fromUtf8("0.00 Hz"));
        if (m_powerButton) {
            m_powerButton->setText(QString());
            m_powerButton->setIcon(QIcon(QStringLiteral(":/image/resources/power.png")));
        }
        m_loadBar->setValue(0);
        updateStateBadge(false);
    }
    emitPowerSummary();
}

void PowerManagementPage::updateStateBadge(bool)
{
}

void PowerManagementPage::emitPowerSummary()
{
    if (!m_poweredOn) {
        emit powerSummaryChanged(QString::fromUtf8("输出关闭 / 设备待机"),
                                 QString::fromUtf8("OFF"),
                                 true);
        return;
    }

    const QString meta = QString::fromUtf8("DC %1V / AC %2V / %3Hz")
        .arg(QString::number(m_dcSpin->value(), 'f', 2),
             QString::number(m_acSpin->value(), 'f', 1),
             QString::number(m_freqSpin->value(), 'f', 2));

    emit powerSummaryChanged(meta,
                             QString::fromUtf8("ON"),
                             false);
}
