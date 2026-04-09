#ifndef POWERMANAGEMENTPAGE_H
#define POWERMANAGEMENTPAGE_H

#include <QWidget>

class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QProgressBar;
class QSlider;

class PowerManagementPage : public QWidget
{
    Q_OBJECT
public:
    explicit PowerManagementPage(QWidget *parent = nullptr);

signals:
    void powerStateChanged(bool on);
    void logMessage(const QString &message);
    void outputSettingsApplied(const QString &taskId,
                               const QString &product,
                               const QString &startTime,
                               const QString &result);

    void powerSummaryChanged(const QString &meta,
                             const QString &value,
                             bool warning);

private slots:
    void togglePower();
    void resetValues();
    void applySafePreset();
    void applyOutputSettings();

private:
    void refreshUi();
    void updateStateBadge(bool on);
    void emitPowerSummary();
    QWidget *createSliderControlRow(const QString &labelText,
                                    QDoubleSpinBox *spinBox,
                                    QSlider *slider,
                                    int sliderFactor,
                                    QWidget *parent);

    bool m_poweredOn;
    QLabel *m_dcOutputValue;
    QLabel *m_acOutputValue;
    QLabel *m_freqValue;
    QPushButton *m_powerButton;
    QProgressBar *m_loadBar;
    QDoubleSpinBox *m_dcSpin;
    QDoubleSpinBox *m_acSpin;
    QDoubleSpinBox *m_freqSpin;
};

#endif // POWERMANAGEMENTPAGE_H
