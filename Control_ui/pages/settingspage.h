#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include <QWidget>

class QComboBox;
class QLineEdit;
class QLabel;
class QPushButton;
class QSpinBox;

class SettingsPage : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsPage(QWidget *parent = nullptr);

signals:
    void extraChanged(const QString &value);
    void logMessage(const QString &message);

private slots:
    void applySettings();
    void restoreDefaults();
    void testConnections();

private:
    void refreshSummary();
    bool validateInputs(QString *errorMessage) const;
    QString buildSummaryText() const;

    QLabel *m_summaryValue;
    QLineEdit *m_dbPathEdit;
    QLineEdit *m_timeIpEdit;
    QSpinBox *m_timePortSpin;
    QComboBox *m_acSerialPortCombo;
    QComboBox *m_acBaudCombo;
    QComboBox *m_acParityCombo;
    QComboBox *m_acStopBitsCombo;
    QSpinBox *m_acAddressSpin;
    QComboBox *m_dcSerialPortCombo;
    QComboBox *m_dcBaudCombo;
    QComboBox *m_dcParityCombo;
    QComboBox *m_dcStopBitsCombo;
    QSpinBox *m_dcAddressSpin;
    QLineEdit *m_videoIpEdit;
    QSpinBox *m_videoPortSpin;

    // 保存后的状态缓存
    QString m_appliedSummary;
    bool m_settingsApplied;
};

#endif // SETTINGSPAGE_H
