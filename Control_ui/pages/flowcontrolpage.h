#ifndef FLOWCONTROLPAGE_H
#define FLOWCONTROLPAGE_H

#include <QStringList>
#include <QSet>
#include <QVector>
#include <QWidget>

class QLabel;
class QListWidget;
class QProgressBar;
class QTableWidget;
class QTimer;
class QPushButton;
class QWidget;

class FlowControlPage : public QWidget
{
    Q_OBJECT
public:
    explicit FlowControlPage(QWidget *parent = nullptr);

    QString currentStageModuleId() const;
    bool flowSessionActive() const;
    bool isRunning() const;

signals:
    void phaseChanged(const QString &meta, const QString &value);
    void logMessage(const QString &message);
    void flowSessionChanged(bool active);
    void runningStateChanged(bool running);
    void stagePageChanged(const QString &moduleId, const QString &title);

public slots:
    void startProgress();
    void pauseProgress();
    void nextStage();
    void previousStage();
    void retryCurrentStage();
    void endFlow();
    void loadHistory();
    void saveFlow();
    void advanceProgress();
    void moveStageUp();
    void moveStageDown();
    void applySelectedStage();

private:
    QString stageName(int stageId) const;
    QString stageDescription(int stageId) const;
    QString stageExecutionText(int stageId) const;
    QStringList stageSteps(int stageId) const;
    QString stageModuleId(int stageId) const;

    int currentSelectedRow() const;
    int currentStageRow() const;
    void refreshUi();
    void updateFlowTable();
    void syncSelection();
    void swapStages(int firstRow, int secondRow);
    void markCurrentStageCompleted();
    void notifyStagePageChanged();

    QVector<int> m_stageOrder;
    int m_currentStage;
    int m_progress;
    bool m_running;
    bool m_flowSessionActive;

    QVector<int> m_stageExecutionCounts;
    QSet<int> m_completedStages;
    bool m_currentStageCounted;

    QLabel *m_stageValue;
    QLabel *m_progressValue;
    QLabel *m_stepValue;
    QProgressBar *m_progressBar;
    QTableWidget *m_flowTable;
    QListWidget *m_stepList;
    QTimer *m_phaseTimer;
    QWidget *m_preStartControls;
    QPushButton *m_inlineStartButton;
};

#endif // FLOWCONTROLPAGE_H
