#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include "vdrapi.h"

class QCloseEvent;
class QLineEdit;
class QLabel;
class QTabWidget;
class QTimer;

class IdcPage;
class PdcPage;

class MainWindow : public QWidget
{
public:
    MainWindow();
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event);

private:
    enum ActiveMode {
        ModeNone,
        ModeIdc,
        ModePdc
    };

    void buildUi();

    QString defaultDllPath() const;
    QString defaultSavePath() const;
    void browseLineEdit(QLineEdit *edit, bool fileMode);

    void appendLog(const QString &text);
    void appendIdcLog(const QString &text);
    void appendPdcLog(const QString &text);
    void appendCurrentLog(const QString &text);

    void ensureApiLoaded();
    void applyVersionIfNeeded();
    void updateProgress();

    void onBrowseDll();
    void onBrowseIdcPath();
    void onBrowsePdcPath();
    void onQueryIdcDirs();
    void onSetIdcDir();
    void onStartIdc();
    void onStartPdc();
    void onStop();

    void applyIdcParameters(const QString &ip, int port, const QString &savePath);
    void applyPdcParameters(const QString &ip, int port, const QString &savePath);

private:
    VdrApi m_api;
    ActiveMode m_mode;
    QTimer *m_timer;

    QTabWidget *m_tabs;
    QLineEdit *m_editDll;
    QLineEdit *m_editVersion;
    QLabel *m_labelLoadState;

    IdcPage *m_idcPage;
    PdcPage *m_pdcPage;

    QString m_idcServerIp;
    int m_idcServerPort;
    QString m_pdcServerIp;
    int m_pdcServerPort;
};

#endif
