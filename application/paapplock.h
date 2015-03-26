#ifndef PAAPPLOCK_H
#define PAAPPLOCK_H

#include <QObject>
#include <QLockFile>

class PaAppLock : public QObject
{
    Q_OBJECT
public:
    explicit PaAppLock(QObject *parent = 0,QString appId = QString());
    ~PaAppLock();
    QString applicationId() const { return id; }
    bool existInstance();

private:
    QString id;
    QString lockFileName;
    QLockFile* lockFile;
};

#endif // PAAPPLOCK_H
