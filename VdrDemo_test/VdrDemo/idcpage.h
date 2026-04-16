#ifndef IDCPAGE_H
#define IDCPAGE_H

#include <QWidget>
#include <QStringList>
#include <QList>

class QCheckBox;
class QByteArray;
class QComboBox;
class QDateTime;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;

class IdcPage : public QWidget
{
public:
    explicit IdcPage(QWidget *parent = 0);

    QPushButton *browseButton() const;
    QPushButton *queryButton() const;
    QPushButton *setTimeButton() const;
    QPushButton *startButton() const;
    QPushButton *stopButton() const;

    void setServerInfo(const QString &ip, int port);
    void resetServerInfo();

    QString savePath() const;
    void setSavePath(const QString &path);

    QString currentDir() const;
    int currentDirIndex() const;
    int dirCount() const;
    bool hasDirData() const;

    QString beginDisplayText() const;
    QString rawTimeRangeText() const;
    void clearTimeRange();
    void setTimeRange(const QDateTime &begin, const QDateTime &end);

    QDateTime beginDateTimeForIndex(int index) const;
    QDateTime endDateTimeForIndex(int index) const;

    long selectedTypeMask() const;
    QString selectedTypeText() const;

    void setProgress(int value);
    void appendLog(const QString &text);

    void populateDirComboFromRaw(const QByteArray &raw);
    void showDirDialog();

private:
    QCheckBox *createTypeBox(const QString &text, int value, QWidget *parent);
    void updateSummary();
    QString formatHourTokenStart(const QString &token) const;
    QString formatHourTokenEnd(const QString &token) const;
    QString compactTimeToken(const QString &displayText) const;
    void updateTimeFromCurrentDir();

private:
    QLabel *m_serverInfo;
    QLineEdit *m_savePath;
    QLineEdit *m_begin;
    QLineEdit *m_end;
    QComboBox *m_dir;
    QList<QCheckBox*> m_types;
    QLabel *m_typeSummary;
    QProgressBar *m_progress;
    QPlainTextEdit *m_log;
    QPushButton *m_btnBrowse;
    QPushButton *m_btnQuery;
    QPushButton *m_btnSetDir;
    QPushButton *m_btnStart;
    QPushButton *m_btnStop;
    bool m_updatingDirCombo;
    QStringList m_dirNumbers;
    QStringList m_dirRanges;
    QStringList m_dirBeginTokens;
    QStringList m_dirEndTokens;
};

#endif
