#include "paapplock.h"
#include <QDir>

#if defined(Q_OS_WIN)
#include <QLibrary>
#include <qt_windows.h>
typedef BOOL(WINAPI*PProcessIdToSessionId)(DWORD,DWORD*);
static PProcessIdToSessionId pProcessIdToSessionId = 0;
#endif
#if defined(Q_OS_UNIX) || defined(Q_OS_OS2)
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#endif


PaAppLock::PaAppLock(QObject *parent, QString appId) :
    QObject(parent)
{
    id = appId;
    QString prefix = "pasigapp-";
    if(id.isEmpty()){
        id = "PaAssistant";
#if defined(Q_OS_WIN)
        id = id.toLower();
#endif
    }

    QByteArray idc = id.toUtf8();
    quint16 idNum = qChecksum(idc.constData(), idc.size());
    lockFileName = prefix + id+ QLatin1Char('-') + QString::number(idNum, 16);

#if defined(Q_OS_WIN)
    if (!pProcessIdToSessionId) {
        QLibrary lib("kernel32");
        pProcessIdToSessionId = (PProcessIdToSessionId)lib.resolve("ProcessIdToSessionId");
    }
    if (pProcessIdToSessionId) {
        DWORD sessionId = 0;
        pProcessIdToSessionId(GetCurrentProcessId(), &sessionId);
        lockFileName += QLatin1Char('-') + QString::number(sessionId, 16);
    }
#else
    lockFileName += QLatin1Char('-') + QString::number(::getuid(), 16);
#endif

    QString dirName = QDir::homePath() + "/.PzAssistant/";
    QDir dir;
    if(!dir.exists(dirName))
        dir.mkdir(dirName);
    lockFileName =dirName  + lockFileName;
    lockFile = new QLockFile(lockFileName);
}

PaAppLock::~PaAppLock()
{
    lockFile->unlock();
    delete lockFile;
}

bool PaAppLock::existInstance()
{
    if(lockFile->isLocked())
        return false;

    if(!lockFile->tryLock())
        return true;
    return false;
}
