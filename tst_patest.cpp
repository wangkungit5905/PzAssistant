#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <QTextCodec>

#include "account.h"

class PaTest : public QObject
{
    Q_OBJECT
    
public:
    PaTest();
    
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void testCase1();
    void testCase1_data();

private:
    bool openAccount();
    bool initSubject();
    bool initPzSet();

    Account* account;
};

PaTest::PaTest()
{
}

void PaTest::initTestCase()
{
    QTextCodec::setCodecForTr(QTextCodec::codecForName("utf-8"));
    account = new Account("TestAccount.dat");
}

void PaTest::cleanupTestCase()
{
}

void PaTest::testCase1()
{
    QFETCH(QString, data);
    QVERIFY2(true, "Failure");
}

void PaTest::testCase1_data()
{
    QTest::addColumn<QString>("data");
    QTest::newRow("0") << QString();
}

bool PaTest::openAccount()
{
}

bool PaTest::initSubject()
{
}

bool PaTest::initPzSet()
{
}

QTEST_MAIN(PaTest)

#include "tst_patest.moc"
