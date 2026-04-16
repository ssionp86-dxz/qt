#include "vdrapi.h"
#include <QDir>

VdrApi::VdrApi()
    : m_setVersion(0), m_vdrdownIdc(0), m_vdrdownPdc(0), m_onProgress(0), m_vdrStop(0),
      m_getSpeed(0), m_querySync(0), m_vdrdownQuery(0), m_vdrdownSetDir(0), m_exitLib(0)
{
}

VdrApi::~VdrApi()
{
    unload();
}

bool VdrApi::load(const QString &dllPath)
{
    unload();
    m_lib.setFileName(QDir::toNativeSeparators(dllPath));
    if (!m_lib.load()) {
        m_lastError = m_lib.errorString();
        return false;
    }
    if (!resolveAll()) {
        unload();
        return false;
    }
    m_lastError.clear();
    return true;
}

void VdrApi::unload()
{
    if (m_lib.isLoaded()) {
        m_lib.unload();
    }
    m_setVersion = 0;
    m_vdrdownIdc = 0;
    m_vdrdownPdc = 0;
    m_onProgress = 0;
    m_vdrStop = 0;
    m_getSpeed = 0;
    m_querySync = 0;
    m_vdrdownQuery = 0;
    m_vdrdownSetDir = 0;
    m_exitLib = 0;
}

bool VdrApi::isLoaded() const
{
    return m_lib.isLoaded();
}

QString VdrApi::lastError() const
{
    return m_lastError;
}

bool VdrApi::resolveAll()
{
    m_setVersion = (SET_VERSION)m_lib.resolve("set_version");
    m_vdrdownIdc = (VDRDOWN_IDC)m_lib.resolve("vdrdown_idc");
    m_vdrdownPdc = (VDRDOWN_PDC)m_lib.resolve("vdrdown_pdc");
    m_onProgress = (ONPROGRESS)m_lib.resolve("onprogress");
    m_vdrStop = (VDRSTOP)m_lib.resolve("vdrstop");
    m_getSpeed = (GETSPEED)m_lib.resolve("getspeed");
    m_querySync = (QUERY_SYNC)m_lib.resolve("queryDiskSyncStatus");
    m_vdrdownQuery = (VDRDOWN_QUERY)m_lib.resolve("vdrdown_query");
    m_vdrdownSetDir = (VDRDOWN_SET_DIR)m_lib.resolve("vdrdown_set_dir");
    m_exitLib = (EXITLIB)m_lib.resolve("exitlib");

    if (!m_vdrdownIdc || !m_vdrdownPdc || !m_onProgress || !m_vdrStop || !m_getSpeed || !m_querySync) {
        m_lastError = QString::fromLocal8Bit("DLL 导出函数不完整，请确认库版本。");
        return false;
    }
    return true;
}

QByteArray VdrApi::toLocal8BitBuffer(const QString &text) const
{
    return QDir::toNativeSeparators(text).toLocal8Bit();
}

int VdrApi::setVersion(int ver)
{
    if (!m_setVersion) return -1;
    return m_setVersion(ver);
}

int VdrApi::startIdc(const QString &savePath, const QString &beginTime, const QString &endTime, long downType)
{
    if (!m_vdrdownIdc) return -1;
    QByteArray path = toLocal8BitBuffer(savePath);
    QByteArray begin = beginTime.trimmed().toLocal8Bit();
    QByteArray end = endTime.trimmed().toLocal8Bit();
    return m_vdrdownIdc(path.data(), begin.data(), end.data(), downType);
}

int VdrApi::startPdc(const QString &savePath, int dataSizeMb, long downType)
{
    if (!m_vdrdownPdc) return -1;
    QByteArray path = toLocal8BitBuffer(savePath);
    return m_vdrdownPdc(path.data(), dataSizeMb, downType);
}

int VdrApi::stop()
{
    if (!m_vdrStop) return -1;
    return m_vdrStop();
}

void VdrApi::progress(QString &filePath, int &percent)
{
    filePath.clear();
    percent = 0;
    if (!m_onProgress) return;
    char buf[1024] = {0};
    m_onProgress(buf, sizeof(buf) - 1, &percent);
    filePath = QString::fromLocal8Bit(buf);
}

int VdrApi::speed()
{
    if (!m_getSpeed) return -1;
    return m_getSpeed();
}

int VdrApi::querySyncStatus(bool isIdc)
{
    if (!m_querySync) return -1;
    return m_querySync(isIdc ? 1 : 0);
}

int VdrApi::queryIdcDirs(int &mode, int &count, QByteArray &rawMap)
{
    mode = 0;
    count = 0;
    rawMap.fill(0, 8192);
    if (!m_vdrdownQuery) return -1;
    int ret = m_vdrdownQuery(&mode, &count, rawMap.data(), rawMap.size());
    int endPos = rawMap.indexOf('\0');
    if (endPos >= 0) rawMap.truncate(endPos);
    return ret;
}

int VdrApi::setIdcDir(const QString &dir)
{
    if (!m_vdrdownSetDir) return -1;
    QByteArray data = dir.toLocal8Bit();
    return m_vdrdownSetDir(data.data());
}

void VdrApi::exitLib()
{
    if (m_exitLib) {
        m_exitLib();
    }
}
