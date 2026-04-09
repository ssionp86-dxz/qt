#ifndef SUBCLOCKMEASUREMENTPAGE_H
#define SUBCLOCKMEASUREMENTPAGE_H

#include <QDateTime>
#include <QString>
#include <QVector>
#include <QWidget>

class QLabel;
class QProgressBar;
class QPushButton;
class QTabBar;
class QTableWidget;
class RackCanvasWidget;
class ClockPreviewWidget;

enum class SubClockShape {
    RoundClock,
    SquareClock
};

enum class SubClockState {
    Uninstalled,
    Pending,
    Passed,
    Failed
};

struct SubClockInfo {
    QString displayName;
    int rackIndex;
    int indexInRack;
    int row;
    int column;
    int span;
    SubClockShape shape;
    SubClockState state;
    double offsetMs;
    QString resultText;
    QString evidencePath;
    QString databaseId;
    QString remark;
    QDateTime lastMeasured;
    bool active;
};

class SubClockMeasurementPage : public QWidget
{
    Q_OBJECT
public:
    explicit SubClockMeasurementPage(QWidget *parent = nullptr);

signals:
    void videoStateChanged(bool running);
    void logMessage(const QString &message);

private slots:
    void startPreview();
    void resetCurrentRackToPending();
    void clearCurrentRack();
    void exportResult();
    void handleTableSelectionChanged();
    void showSelectedClockZoom();
    void setSelectedAsRoundClock();
    void setSelectedAsSquareClock();
    void removeSelectedClock();

private:
    void initialiseRackModel();
    void buildUi();
    void refreshUi();
    void refreshMetrics();
    void refreshRackButtons();
    void refreshRackCanvas();
    void refreshSelectionPanel();
    void refreshTable();

    void selectRack(int rackIndex);
    void selectSlot(int rackIndex, int row, int col, bool openDialogIfEmpty = false);
    int selectedClockIndex() const;
    int clockIndexAt(int rackIndex, int row, int col) const;
    QVector<int> clockIndexesForRack(int rackIndex) const;

    void showSlotContextMenu(int rackIndex, int row, int col);
    void openInstallDialogForSlot(int rackIndex, int row, int col);
    void showSquareClockInstallUnavailableMessage(int rackIndex, int row, int col);
    bool canPlaceSquareClock(int rackIndex, int row, int col, int ignoreClockIndex = -1) const;
    void installClock(int rackIndex, int row, int col, SubClockShape shape);
    void convertClockShape(int clockIndex, SubClockShape shape);
    void removeClock(int clockIndex);
    void testRack(int rackIndex);
    void measureClock(SubClockInfo &clock);

    int totalInstalledCount() const;
    int totalPendingCount() const;
    int totalPassedCount() const;
    int totalFailedCount() const;

    int occupiedPointCount(int rackIndex) const;
    int rackPendingCount(int rackIndex) const;
    int rackPassedCount(int rackIndex) const;
    int rackFailedCount(int rackIndex) const;

    QString currentSelectionText() const;
    QString rackSummaryText(int rackIndex) const;

    bool m_running;
    bool m_syncingSelection;
    bool m_refreshingTable;
    int m_currentRackIndex;
    int m_selectedRow;
    int m_selectedCol;
    int m_snapshotSequence;

    QLabel *m_rackMetricValue;
    QLabel *m_installMetricValue;
    QLabel *m_resultMetricValue;
    QLabel *m_selectionMetricValue;
    QLabel *m_selectedTitleLabel;
    QLabel *m_selectedMetaLabel;
    QLabel *m_selectedHintLabel;
    QLabel *m_tableTitleLabel;

    QProgressBar *m_progressBar;
    QTableWidget *m_resultTable;
    ClockPreviewWidget *m_previewWidget;
    RackCanvasWidget *m_rackCanvas;
    QTabBar *m_rackTabBar;

    QVector<SubClockInfo> m_clocks;
    QVector<QVector<int> > m_slotOwners;
};

#endif // SUBCLOCKMEASUREMENTPAGE_H
