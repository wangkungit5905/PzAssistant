#include "accountpropertyconfig.h"


#include <QListWidget>
#include <QStackedWidget>
#include <QHBoxLayout>
#include <QPushButton>

ApcBase::ApcBase(Account *account, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ApcBase)
{
    ui->setupUi(this);
}

ApcBase::~ApcBase()
{
    delete ui;
}

//////////////////////////////ApcSuite////////////////////////////////////////////////
ApcSuite::ApcSuite(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ApcSuite)
{
    ui->setupUi(this);
}

ApcSuite::~ApcSuite()
{
    delete ui;
}

////////////////////////////ApcBank///////////////////////////////////////////////
ApcBank::ApcBank(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ApcBank)
{
    ui->setupUi(this);
}

ApcBank::~ApcBank()
{
    delete ui;
}

///////////////////////////ApcSubject////////////////////////////////////////////
ApcSubject::ApcSubject(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ApcSubject)
{
    ui->setupUi(this);
}

ApcSubject::~ApcSubject()
{
    delete ui;
}

///////////////////////////////ApcReport/////////////////////////////////////////
ApcReport::ApcReport(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ApcReport)
{
    ui->setupUi(this);
}

ApcReport::~ApcReport()
{
    delete ui;
}

//////////////////////////ApcLog/////////////////////////////////////////
ApcLog::ApcLog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ApcLog)
{
    ui->setupUi(this);
}

ApcLog::~ApcLog()
{
    delete ui;
}

///////////////////////////////AccountPropertyConfig///////////////////////////////////////
AccountPropertyConfig::AccountPropertyConfig(Account* account, QWidget *parent) : QDialog(parent)
{
    contentsWidget = new QListWidget;
    contentsWidget->setViewMode(QListView::IconMode);
    contentsWidget->setIconSize(QSize(48, 48));
    contentsWidget->setMovement(QListView::Static);
    contentsWidget->setMaximumWidth(86);
    contentsWidget->setSpacing(12);

    pagesWidget = new QStackedWidget;
    pagesWidget->addWidget(new ApcBase(account));
    pagesWidget->addWidget(new ApcSuite);
    pagesWidget->addWidget(new ApcBank);
    pagesWidget->addWidget(new ApcSubject);
    pagesWidget->addWidget(new ApcReport);
    pagesWidget->addWidget(new ApcLog);

    QPushButton *closeButton = new QPushButton(tr("关闭"));

    createIcons();
    contentsWidget->setCurrentRow(0);

    connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));

    QHBoxLayout *horizontalLayout = new QHBoxLayout;
    horizontalLayout->addWidget(contentsWidget);
    horizontalLayout->addWidget(pagesWidget, 1);

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(closeButton);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(horizontalLayout,1);
    //mainLayout->addStretch(1);
    mainLayout->addSpacing(12);
    mainLayout->addLayout(buttonsLayout);
    setLayout(mainLayout);

    setWindowTitle(tr("账户属性配置"));
}

AccountPropertyConfig::~AccountPropertyConfig()
{

}

void AccountPropertyConfig::pageChanged(int index)
{
    pagesWidget->setCurrentIndex(index);
}

void AccountPropertyConfig::createIcons()
{
    QListWidgetItem *item;
    item = new QListWidgetItem(contentsWidget);
    item->setIcon(QIcon(":/images/accProperty/baseInfo.png"));
    item->setText(tr("基本"));
    item->setTextAlignment(Qt::AlignHCenter);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    item = new QListWidgetItem(contentsWidget);
    item->setIcon(QIcon(":/images/accProperty/suiteInfo.png"));
    item->setText(tr("帐套"));
    item->setTextAlignment(Qt::AlignHCenter);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    item = new QListWidgetItem(contentsWidget);
    item->setIcon(QIcon(":/images/accProperty/bankInfo.png"));
    item->setText(tr("开户行"));
    item->setTextAlignment(Qt::AlignHCenter);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    item = new QListWidgetItem(contentsWidget);
    item->setIcon(QIcon(":/images/accProperty/subjectInfo.png"));
    item->setText(tr("科目"));
    item->setTextAlignment(Qt::AlignHCenter);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    item = new QListWidgetItem(contentsWidget);
    item->setIcon(QIcon(":/images/accProperty/reportInfo.png"));
    item->setText(tr("报表"));
    item->setTextAlignment(Qt::AlignHCenter);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    item = new QListWidgetItem(contentsWidget);
    item->setIcon(QIcon(":/images/accProperty/logInfo.png"));
    item->setText(tr("日志"));
    item->setTextAlignment(Qt::AlignHCenter);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    connect(contentsWidget,SIGNAL(currentRowChanged(int)),
         this, SLOT(pageChanged(int)));
}
