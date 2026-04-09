#ifndef OFFSETMEASUREMENTPAGE_H
#define OFFSETMEASUREMENTPAGE_H

#include <QVector>
#include <QWidget>

class QLabel;
class QProgressBar;
class QPushButton;
class QRadioButton;
class QTableWidget;
class QTimer;
class TrendChartWidget;
class QWidget;

class OffsetMeasurementPage : public QWidget
{
    Q_OBJECT
public:
    explicit OffsetMeasurementPage(QWidget *parent = nullptr);

signals:
    void measurementChanged(const QString &summary);
    void logMessage(const QString &message);

private slots:
    void startOrStopMeasurement();
    void repeatMeasurement();
    void updateMeasurement();
    void handleChannelCheckChanged(bool checked);
    void selectAllChannels();
    void updateModeSelection();

private:
    void refreshChannelTable();
    void refreshUi();
    bool hasEnabledChannels() const;
    void resetHistory();
    void repositionEnableHeaderControls();
    void updateSelectAllState();
    bool areAllChannelsChecked() const;
    void setAllChannelsChecked(bool checked);

    bool m_continuousMode;
    bool m_measuring;
    int m_elapsedSeconds;
    QVector<double> m_historySamples;
    QVector<QVector<double> > m_channelHistorySamples;
    QLabel *m_deltaValue;
    QLabel *m_sigmaValue;
    QLabel *m_modeValue;
    QLabel *m_durationValue;
    QLabel *m_stateValue;
    QProgressBar *m_deltaBar;
    QPushButton *m_startButton;
    QPushButton *m_repeatButton;
    QPushButton *m_selectAllButton;
    QWidget *m_enableHeaderWidget;
    QLabel *m_enableHeaderLabel;
    QRadioButton *m_singleRadio;
    QRadioButton *m_continuousRadio;
    QTableWidget *m_channelTable;
    TrendChartWidget *m_trendChart;
    QTimer *m_measureTimer;
    QVector<bool> m_channelEnabled;
    QVector<double> m_lastSamples;
};

#endif // OFFSETMEASUREMENTPAGE_H
