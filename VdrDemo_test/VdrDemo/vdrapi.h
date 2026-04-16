#ifndef VDRAPI_H
#define VDRAPI_H

#include <QString>
#include <QLibrary>
#include "vdr_glk.h"

class VdrApi
{
public:
    VdrApi();
    ~VdrApi();

    bool load(const QString &dllPath);
    void unload();
    bool isLoaded() const;
    QString lastError() const;

    int setVersion(int ver);
    int startIdc(const QString &savePath, const QString &beginTime, const QString &endTime, long downType);
    int startPdc(const QString &savePath, int dataSizeMb, long downType);
    int stop();
    void progress(QString &filePath, int &percent);
    int speed();
    int querySyncStatus(bool isIdc);
    int queryIdcDirs(int &mode, int &count, QByteArray &rawMap);
    int setIdcDir(const QString &dir);
    void exitLib();

private:
    typedef int (*SET_VERSION)(int ver);
    typedef int (*VDRDOWN_IDC)(char* path, char* begintime, char* endtime, long downtype);
    typedef int (*VDRDOWN_PDC)(char* path, int datasize, long downtype);
    typedef void (*ONPROGRESS)(char* filepath, int size, int *percent);
    typedef int (*VDRSTOP)();
    typedef int (*GETSPEED)();
    typedef int (*QUERY_SYNC)(int);
    typedef int (*VDRDOWN_QUERY)(int *mode, int *num, char *map, int mapsize);
    typedef int (*VDRDOWN_SET_DIR)(char *dir);
    typedef void (*EXITLIB)();

    bool resolveAll();
    QByteArray toLocal8BitBuffer(const QString &text) const;

private:
    QLibrary m_lib;
    QString m_lastError;

    SET_VERSION m_setVersion;
    VDRDOWN_IDC m_vdrdownIdc;
    VDRDOWN_PDC m_vdrdownPdc;
    ONPROGRESS m_onProgress;
    VDRSTOP m_vdrStop;
    GETSPEED m_getSpeed;
    QUERY_SYNC m_querySync;
    VDRDOWN_QUERY m_vdrdownQuery;
    VDRDOWN_SET_DIR m_vdrdownSetDir;
    EXITLIB m_exitLib;
};

#endif
