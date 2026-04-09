#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QList>
#include <QMainWindow>
#include <QMap>
#include <QPoint>

class QEvent;
class QMouseEvent;

#include "moduleinfo.h"

class QLabel;
class QPushButton;
class QStackedWidget;
class QWidget;
class PreviewCardWidget;
class PowerManagementPage;
class TimingManagementPage;
class SubClockMeasurementPage;
class OffsetMeasurementPage;
class DataManagementPage;
class FlowControlPage;
class SettingsPage;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void switchModule(const QString &moduleId);
    void handlePowerStateChanged(bool on);
    void handleClockChanged(const QString &clockText);
    void handleVideoStateChanged(bool online);
    void handleMeasurementChanged(const QString &summary);
    void handleStorageStateChanged(const QString &meta, const QString &value, const QString &writeRate);
    void handlePhaseChanged(const QString &meta, const QString &value);
    void handleExtraChanged(const QString &value);
    void handleFlowSessionChanged(bool active);
    void handleFlowRunningStateChanged(bool running);
    void handleFlowStagePageChanged(const QString &moduleId, const QString &title);
    void appendLog(const QString &message);

private:
    void buildUi();
    QWidget *buildTitleBar();
    QWidget *buildPreviewPanel();
    QWidget *buildDetailPanel();
    QWidget *buildFlowToolbar(QWidget *parent);
    PreviewCardWidget *createPreviewCard(const ModuleInfo &info, QWidget *parent);
    void updateCard(const QString &id, const QString &meta, const QString &value, bool warning);
    void setCurrentModule(const QString &id);
    void updateTaskButtonState(QPushButton *button, bool active);
    QPushButton *createWindowButton(const QString &text, const QString &objectName);
    void updateDetailHeader(const QString &id, const QString &overrideTitle = QString());
    QList<ModuleInfo> initialModules() const;

    QMap<QString, PreviewCardWidget *> m_cards;
    QMap<QString, int> m_pageIndex;
    QStackedWidget *m_stackedWidget;
    QLabel *m_detailTitleLabel;
    QLabel *m_detailSubtitleLabel;
    QLabel *m_footerStateLabel;
    QLabel *m_footerClockLabel;
    QLabel *m_footerWriteRateLabel;
    QPushButton *m_dataButton;
    QPushButton *m_settingsButton;
    QWidget *m_titleBarWidget;
    QWidget *m_flowToolbar;
    QPushButton *m_flowToolbarPauseButton;
    bool m_draggingWindow;
    QPoint m_dragOffset;

    PowerManagementPage *m_powerManagementPage;
    TimingManagementPage *m_timingManagementPage;
    SubClockMeasurementPage *m_subClockMeasurementPage;
    OffsetMeasurementPage *m_offsetMeasurementPage;
    DataManagementPage *m_dataManagementPage;
    FlowControlPage *m_flowControlPage;
    SettingsPage *m_settingsPage;
};

#endif // MAINWINDOW_H
