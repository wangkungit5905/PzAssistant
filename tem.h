#ifndef TEM_H
#define TEM_H

#include <QSqlDatabase>
#include <QString>

void tranUsdToRbm(int y,int m,int fid,QSqlDatabase db);
void delSpecPidPz(int pid);

#endif // TEM_H
