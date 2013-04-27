#include <QtGui/QApplication>
#include <QtCore/QTextCodec>
#include <QtTest>

#include "tst_patest.h"
#include "testpzsetstat.h"
#include "../account.h"
#include "../global.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QTextCodec::setCodecForTr(QTextCodec::codecForName("utf-8"));
    appInit();
    Account* account = new Account("TestAccount.dat");
    //TestExtraFun* pt = new TestExtraFun(account);
    //TestPzObj* pt = new TestPzObj(account);
    TestPzSetStat* pt = new TestPzSetStat(account);
    QTest::qExec(pt);
    delete pt;
    account->close();
    delete account;
    appExit();
    return app.exec();
}


