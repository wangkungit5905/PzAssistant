#include "aboutdialog.h"
#include "securitys.h"
#include "config.h"
#include "global.h"
#include "mainapplication.h"
#include "VersionRev.h"

#include <QTabWidget>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QFile>
#include <QTextBrowser>
#include<QCoreApplication>


AboutDialog::AboutDialog(QWidget *parent):PaDialog(parent,Qt::MSWindowsFixedSizeDialogHint)
{
    setWindowTitle(tr("关于..."));
    setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setObjectName("AboutDialog");
    setMinimumWidth(480);

    QTabWidget *tabWidget = new QTabWidget();

    //版本信息页
    QString revisionStr;
    if (!QString(BUILD_STR).isEmpty()) {
        revisionStr = "<BR>" + tr("Revision") + " " + QString("%1").arg(BUILD_STR);
    }
    versionStr = QString("%1.%2.%3.%4").arg(VER_MASTE).arg(VER_SECOND)
            .arg(VER_REVISION).arg(BUILD_NUMBER);
    QString appInfo =
          "<html><style>a { color: blue; text-decoration: none; }</style><body>"
          "<CENTER>"
          "<IMG SRC=\":/images/accSuite.png\">"
          "<P>"
          + tr("版本：") + " " + "<B>" + QString(versionStr) + "</B>" + QString(" (%1)").arg(APP_BUILD_DATE)
          + revisionStr
          + "</P>"
          + "<BR>"
          + tr("凭证助手是一个开源的跨平台的财务辅助处理软件。")
          + "<P>" + tr("它使用如下开源软件：")
          + QString(" Qt-%1").arg(QT_VERSION_STR)
          + "<BR></P>"
          //+ QString("<a href=\"%1\">%1</a>").arg("http://pzassistant.org") +
          "<P>版权： 2011-2015 SSC "
          + QString("<a href=\"%1\">E-mail</a>").arg("mailto:wangkungl98@gmail.com") + "</P>"
          "</CENTER></body></html>";
    QLabel *infoLabel = new QLabel(appInfo);
    infoLabel->setOpenExternalLinks(true);
    infoLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);

    QHBoxLayout *mainLayout = new QHBoxLayout();
    mainLayout->addWidget(infoLabel);
    QWidget *mainWidget = new QWidget();
    mainWidget->setLayout(mainLayout);

    //站点信息页
    QGridLayout *wsLayout = new QGridLayout;
    AppConfig* appCfg = AppConfig::getInstance();
    QHash<int,QString> osTypes;
    appCfg->getOsTypes(osTypes);
    WorkStation* mac = AppConfig::getInstance()->getLocalStation();
    QLabel *t = new QLabel(tr("站点ID"),this);
    QLineEdit *e = new QLineEdit(QString::number(mac->getMID()),this);
    e->setReadOnly(true);
    wsLayout->addWidget(t,0,0); wsLayout->addWidget(e,0,1);
    t = new QLabel(tr("平台类型"),this);
    e = new QLineEdit(osTypes.value(mac->osType()),this);
    e->setReadOnly(true);
    wsLayout->addWidget(t,1,0); wsLayout->addWidget(e,1,1);
    t = new QLabel(tr("站点简称"),this);
    e = new QLineEdit(mac->name(),this);
    e->setReadOnly(true);
    wsLayout->addWidget(t,2,0); wsLayout->addWidget(e,2,1);
    t = new QLabel(tr("站点全称"),this);
    e = new QLineEdit(mac->description(),this);
    e->setReadOnly(true);
    wsLayout->addWidget(t,3,0); wsLayout->addWidget(e,3,1);
    QWidget *wsWidget = new QWidget();
    wsWidget->setLayout(wsLayout);

    //作者页
    QTextEdit *authorsTextEdit = new QTextEdit(this);
    authorsTextEdit->setReadOnly(true);
    QFile file;
    file.setFileName(":/files/files/AUTHORS");
    file.open(QFile::ReadOnly);
    authorsTextEdit->setText(QString::fromUtf8(file.readAll()));
    file.close();

    QHBoxLayout *authorsLayout = new QHBoxLayout();
    authorsLayout->addWidget(authorsTextEdit);
    QWidget *authorsWidget = new QWidget();
    authorsWidget->setLayout(authorsLayout);

    //版本历史页
    QTextBrowser *historyTextBrowser = new QTextBrowser();
    historyTextBrowser->setOpenExternalLinks(true);
    file.setFileName(":/files/files/HISTORY");
    file.open(QFile::ReadOnly);
    historyTextBrowser->setHtml(QString::fromUtf8(file.readAll()));
    file.close();

    QHBoxLayout *historyLayout = new QHBoxLayout();
    historyLayout->addWidget(historyTextBrowser);
    QWidget *historyWidget = new QWidget();
    historyWidget->setLayout(historyLayout);

    //版权页
    QTextEdit *licenseTextEdit = new QTextEdit();
    licenseTextEdit->setReadOnly(true);
    file.setFileName(":/files/files/COPYING");
    file.open(QFile::ReadOnly);
    QString str = QString(QString::fromUtf8(file.readAll()));
    licenseTextEdit->setText(str);
    file.close();

    QHBoxLayout *licenseLayout = new QHBoxLayout();
    licenseLayout->addWidget(licenseTextEdit);
    QWidget *licenseWidget = new QWidget();
    licenseWidget->setLayout(licenseLayout);

    //信息页
    QString information =
          "<p><b>应用信息</b>"
          "<table border=\"0\">"
          "<tr></tr><tr>"
          "<td>" + tr("应用程序目录：") + " </td>"
          "<td>" + QCoreApplication::applicationDirPath() + "</td></tr>"
          "<tr><td>"+ tr("账户目录:") + " </td>"
          "<td>" + DATABASE_PATH + "</td></tr>"
          "<tr><td>"+ tr("备份目录:") + "</td>"
          "<td>" + BACKUP_PATH + "</td></tr>"
          "<tr>""<td>" + tr("基本库目录:") + " </td>"
          "<td>" + BASEDATA_PATH + "</td></tr>"

          "<tr></tr>"
          "<tr><td>" + tr("设置文件:") + " </td>"
          "<td>" + mainApp->settingFile() + "</td></tr>"
          "<tr><td>" + tr("日志文件：") + " </td>"
          "<td>" + LOGS_PATH + "app.log" + "</td>"
          "</tr></table>"
          "<p><b>本地缓存账户</b>"
          "<tr><td>代码</td><td>账户全称</td></tr>";
    foreach(AccountCacheItem * item, appCfg->getAllCachedAccounts()){
        information.append(QString("<tr><td>%1</td><td>%2</td></tr>")
                           .arg(item->code).arg(item->accLName));
    }
    information.append("</table>");

    QTextEdit *informationTextEdit = new QTextEdit();
    informationTextEdit->setReadOnly(true);
    informationTextEdit->setText(information);

    QHBoxLayout *informationLayout = new QHBoxLayout();
    informationLayout->addWidget(informationTextEdit);

    QWidget *informationWidget = new QWidget();
    informationWidget->setLayout(informationLayout);

    tabWidget->addTab(mainWidget, tr("版本"));
    tabWidget->addTab(wsWidget, tr("工作站"));
    tabWidget->addTab(authorsWidget,tr("作者"));
    tabWidget->addTab(historyWidget,tr("历史"));
    tabWidget->addTab(licenseWidget,tr("版权"));
    tabWidget->addTab(informationWidget,tr("信息"));

    pageLayout->addWidget(tabWidget);
    buttonBox->addButton(QDialogButtonBox::Close);
}

AboutDialog::~AboutDialog()
{

}

