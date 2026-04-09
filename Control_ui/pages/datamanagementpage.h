#ifndef DATAMANAGEMENTPAGE_H
#define DATAMANAGEMENTPAGE_H

#include <QWidget>

class QLabel;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QProgressBar;
class QTableWidget;

class DataManagementPage : public QWidget
{
    Q_OBJECT
public:
    explicit DataManagementPage(QWidget *parent = nullptr);
    void appendRuntimeLog(const QString &message);

    void appendTestRecord(const QString &taskId,
                          const QString &product,
                          const QString &startTime,
                          const QString &result);

    void appendHistoryEntry(const QString &entryName);
    void updateStorageRuntime(int usedGb,
                              int writeRateMb,
                              bool recording,
                              const QString &lastAction = QString());

signals:
    void logMessage(const QString &message);
    void storageStateChanged(const QString &meta, const QString &value, const QString &writeRate);

private slots:
    void toggleRecording();
    void openArchive();
    void analyzeStorage();
    void exportLog();
    void exportReport();

private:
    void refreshUi();
    void setRecordingState(bool recording, const QString &reason);
    void appendHistoryLog(const QString &fileName);
    void prependRecord(const QString &taskId,
                       const QString &product,
                       const QString &startTime,
                       const QString &result);
    QString buildStorageSummary() const;
    QString currentWriteRateText() const;

    bool m_recording;
    bool m_transitioning;
    int m_usedGb;
    int m_recordSequence;
    int m_currentWriteRate;

    QLabel *m_storagePoolValue;
    QLabel *m_usageValue;
    QLabel *m_freeValue;
    QLabel *m_healthValue;
    QLabel *m_captureStateValue;
    QLabel *m_writeSpeedValue;
    QLabel *m_lastActionValue;

    QPushButton *m_recordButton;
    QProgressBar *m_usageBar;
    QTableWidget *m_recordTable;
    QPlainTextEdit *m_runtimeLogEdit;
    QListWidget *m_historyLogList;
};

#endif // DATAMANAGEMENTPAGE_H
