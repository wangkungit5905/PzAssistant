#include "paapplock.h"

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

namespace QtLP_Private {
#include "qtlockedfile.cpp"
#if defined(Q_OS_WIN)
#include "qtlockedfile_win.cpp"
#else
#include "qtlockedfile_unix.cpp"
#endif
}


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

    lockFile.setFileName(lockFileName);
    lockFile.open(QIODevice::ReadWrite);
}

bool PaAppLock::existInstance()
{
    if (lockFile.isLocked())
        return false;

    if (!lockFile.lock(QtLP_Private::QtLockedFile::WriteLock, false))
        return true;

//    bool res = server->listen(socketName);
//#if defined(Q_OS_UNIX) && (QT_VERSION >= QT_VERSION_CHECK(4,5,0))
//    // ### Workaround
//    if (!res && server->serverError() == QAbstractSocket::AddressInUseError) {
//        QFile::remove(QDir::cleanPath(QDir::tempPath())+QLatin1Char('/')+socketName);
//        res = server->listen(socketName);
//    }
//#endif
//    if (!res)
//        qWarning("QtSingleCoreApplication: listen on local socket failed, %s", qPrintable(server->errorString()));
//    QObject::connect(server, SIGNAL(newConnection()), SLOT(receiveConnection()));
    return false;
}
