#ifndef PAAPPLOCK_H
#define PAAPPLOCK_H

#include <QObject>
#include "qtlockedfile.h"

class PaAppLock : public QObject
{
    Q_OBJECT
public:
    explicit PaAppLock(QObject *parent = 0,QString appId = QString());
    QString applicationId() const { return id; }
    bool existInstance();

private:
    QString id;
    QString lockFileName;
    QtLP_Private::QtLockedFile lockFile;
};

#endif // PAAPPLOCK_H
