#ifndef TIMINGMANAGEMENTPAGE_H
#define TIMINGMANAGEMENTPAGE_H

#include <QWidget>

class QCheckBox;
class QComboBox;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QTableWidget;
class QWidget;

class TimingManagementPage : public QWidget
{
    Q_OBJECT
public:
    explicit TimingManagementPage(QWidget *parent = nullptr);

    void setTimingStatus(const QString &sourceText,
                         bool ntpEnabled,
                         bool outputsEnabled,
                         bool synced,
                         const QString &currentTimeText,
                         const QString &lastSyncText);

    void clearTimingStatus();
    void initializeChannelTable();
    void clearChannelContents();
    void setChannelRowData(int row,
                           const QString &content,
                           const QString &phaseOffsetNs,
                           const QString &jitterText,
                           const QString &stateText);

signals:
    void clockChanged(const QString &clockText);
    void logMessage(const QString &message);
    void timingOutputApplied(const QString &taskId,
                             const QString &product,
                             const QString &startTime,
                             const QString &result);
    void syncRequested();
    void ntpToggleRequested(bool enabled);
    void sourceSwitchRequested(const QString &targetMode);
    void outputsToggleRequested(bool enabled);

private slots:
    void syncNow();
    void toggleNtp();
    void switchSource();
    void applyOutputConfig();
    void handleFormatChanged(const QString &formatText);
    void handleSelectAllButtonClicked();
    void handleChannelCheckToggled(bool checked);

private:
    void refreshUi();
    void refreshButtonIcons();
    void initializeIconButton(QPushButton *button, const QString &toolTipText);
    QString selectedFormatText() const;
    void repositionEnableHeaderControls();
    void updateSelectAllState();
    bool areAllChannelsChecked() const;
    void setAllChannelsChecked(bool checked);

    QString m_sourceText;
    bool m_ntpEnabled;
    bool m_outputsEnabled;
    bool m_synced;

    QString m_currentTimeText;
    QString m_lastSyncText;

    QLabel *m_sourceDescriptionLabel;
    QLabel *m_syncDescriptionLabel;
    QLabel *m_ntpDescriptionLabel;
    QTableWidget *m_outputTable;
    QComboBox *m_todFormatCombo;
    QPlainTextEdit *m_todEditor;
    QPushButton *m_sourceButton;
    QPushButton *m_syncButton;
    QPushButton *m_ntpButton;
    QPushButton *m_applyButton;
    QPushButton *m_selectAllButton;
    QWidget *m_enableHeaderWidget;
    QLabel *m_enableHeaderLabel;
};

#endif // TIMINGMANAGEMENTPAGE_H
