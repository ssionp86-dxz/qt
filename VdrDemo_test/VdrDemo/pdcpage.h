#ifndef PDCPAGE_H
#define PDCPAGE_H

#include <QWidget>
#include <QList>
#include <QString>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QSpinBox;

class PdcPage : public QWidget
{
public:
    explicit PdcPage(QWidget *parent = 0);

    QPushButton *browseButton() const;
    QPushButton *startButton() const;
    QPushButton *stopButton() const;

    void setServerInfo(const QString &ip, int port);
    void resetServerInfo();

    QString savePath() const;
    void setSavePath(const QString &path);

    int dataSizeMb() const;

    long selectedTypeMask() const;
    QString selectedTypeText() const;

    void setProgress(int value);
    void appendLog(const QString &text);

private:
    QCheckBox *createTypeBox(const QString &text, int value, QWidget *parent);
    void updateSummary();

private:
    QLabel *m_serverInfo;
    QLineEdit *m_savePath;
    QSpinBox *m_size;
    QList<QCheckBox*> m_types;
    QLabel *m_typeSummary;
    QProgressBar *m_progress;
    QPlainTextEdit *m_log;
    QPushButton *m_btnBrowse;
    QPushButton *m_btnStart;
    QPushButton *m_btnStop;
};

#endif
