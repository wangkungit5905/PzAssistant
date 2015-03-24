#-------------------------------------------------
#
# Project created by QtCreator 2011-01-19T12:59:18
#
#-------------------------------------------------
# VCS revision info
REVFILE = VersionRev.h
QMAKE_DISTCLEAN += $$REVFILE
# tortoisehg-2.8 running in win32 hangs with 3 \ in sources, but works fine in linux
THG_WIN32_FIXME = '\\\"'

REVISION_NUM = $$system(git rev-list --count HEAD)
count(REVISION_NUM, 1) {
    VERSION_REV = git-$$REVISION_NUM-$$system(git rev-parse --short HEAD)
} else {
    VERSION_REV = 0
}
message(VCS revision: $$VERSION_REV BuildNumber: $$REVISION_NUM)
win32 {
    system(echo $${LITERAL_HASH}define REVISION_STR $$VERSION_REV > $$REVFILE)
    system(echo $${LITERAL_HASH}define REVISION_NUMBER $$REVISION_NUM >> $$REVFILE)
} else {
    system(echo \\$${LITERAL_HASH}define REVISION_STR $$THG_WIN32_FIXME$$VERSION_REV$$THG_WIN32_FIXME > $$REVFILE)
    system(echo \\$${LITERAL_HASH}define REVISION_NUMBER $$THG_WIN32_FIXME$$REVISION_NUM$$THG_WIN32_FIXME >> $$REVFILE)
}

QT       += core widgets sql xml printsupport network
win32{
    QT += axcontainer
}

TARGET = PzAssistant
DESTDIR = $${PWD}/../workDir/
TEMPLATE = app

SOURCES += main.cpp\
    config.cpp \
    appmodel.cpp \
    dialogs.cpp \
    utils.cpp \
    widgets.cpp \
    completsubinfodialog.cpp \
    dialog2.cpp \
    mainwindow.cpp \
    printUtils.cpp \
    printetemplate.cpp \
    securitys.cpp \
    global.cpp \
    delegates2.cpp \
    tdpreviewdialog.cpp \
    HierarchicalHeaderView.cpp \
    previewdialog.cpp \
    account.cpp \
    otherModule.cpp \
    dialog3.cpp \
    pz.cpp \
    tables.cpp \
    cal.cpp \
    subjectsearchform.cpp \
    viewpzseterrorform.cpp \
    pzdsform.cpp \
    dbutil.cpp \
    logs/logview.cpp \
    logs/Logger.cpp \
    logs/FileAppender.cpp \
    logs/ConsoleAppender.cpp \
    logs/AbstractStringAppender.cpp \
    logs/AbstractAppender.cpp \
    version.cpp \
    subject.cpp \
    PzSet.cpp \
    statutil.cpp \
    curstatdialog.cpp \
    commands.cpp \
    pzdialog.cpp \
    widgets/bawidgets.cpp \
    delegates.cpp \
    widgets/variousWidgets.cpp \
    statements.cpp \
    accountpropertyconfig.cpp \
    suiteswitchpanel.cpp \
    databaseaccessform.cpp \
    transfers.cpp \
    showdzdialog.cpp \
    widgets/subjectselectorcombobox.cpp \
    testform.cpp \
    widgets/fstsubeditcombobox.cpp \
    nabaseinfodialog.cpp \
    newsndsubdialog.cpp \
    importovaccdlg.cpp \
    optionform.cpp \
    excel/ExcelUtil.cpp \
    taxescomparisonform.cpp \
    outputexceldlg.cpp \
    tools/notemgrform.cpp \
    tools/externaltoolconfigform.cpp \
    seccondialog.cpp \
    frmmessagebox.cpp \
    iconhelper.cpp \
    crtaccountfromolddlg.cpp \
    batchoutputdialog.cpp \
    batchimportdialog.cpp \
    lookysyfitemform.cpp \
    application/mainapplication.cpp \
    application/splashscreen.cpp \
    application/paapplock.cpp \
    application/qtlockedfile.cpp \
    common/padialog.cpp \
    aboutdialog.cpp

HEADERS  += \
    config.h \
    global.h \
    appmodel.h \
    common.h \
    dialogs.h \
    c.h \
    utils.h \
    widgets.h \
    completsubinfodialog.h \
    dialog2.h \
    mainwindow.h \
    printUtils.h \
    printtemplate.h \
    commdatastruct.h \
    securitys.h \
    delegates2.h \
    dialog3.h \
    tdpreviewdialog.h \
    HierarchicalHeaderView.h \
    previewdialog.h \
    account.h \
    otherModule.h \
    pz.h \
    tables.h \
    cal.h \
    subjectsearchform.h \
    viewpzseterrorform.h \
    pzdsform.h \
    dbutil.h \
    logs/logview.h \
    logs/LogStruct.h \
    logs/Logger.h \
    logs/FileAppender.h \
    logs/ConsoleAppender.h \
    logs/AbstractStringAppender.h \
    logs/AbstractAppender.h \
    version.h \
    subject.h \
    PzSet.h \
    statutil.h \
    curstatdialog.h \
    commands.h \
    statements.h \
    pzdialog.h \
    widgets/bawidgets.h \
    delegates.h \
    widgets/variousWidgets.h \
    accountpropertyconfig.h \
    configvariablenames.h \
    suiteswitchpanel.h \
    databaseaccessform.h \
    keysequence.h \
    transfers.h \
    showdzdialog.h \
    widgets/subjectselectorcombobox.h \
    testform.h \
    globalVarNames.h \
    widgets/fstsubeditcombobox.h \
    nabaseinfodialog.h \
    newsndsubdialog.h \
    importovaccdlg.h \
    optionform.h \
    excel/ExcelUtil.h \
    taxescomparisonform.h \
    outputexceldlg.h \
    tools/notemgrform.h \
    tools/externaltoolconfigform.h \
    seccondialog.h \
    frmmessagebox.h \
    myhelper.h \
    iconhelper.h \
    crtaccountfromolddlg.h \
    batchoutputdialog.h \
    batchimportdialog.h \
    lookysyfitemform.h \
    application/mainapplication.h \
    application/splashscreen.h \
    application/paapplock.h \
    application/qtlockedfile.h \
    common/padialog.h \
    aboutdialog.h \
    VersionRev.h

FORMS    += \
    forms/createaccountdialog.ui \
    forms/openaccountdialog.ui \
    forms/collectpzdialog.ui \
    forms/reportdialog.ui \
    forms/sqltooldialog.ui \
    forms/completsubinfodialog.ui \
    forms/basummaryform.ui \
    forms/mainwindow.ui \
    forms/ratesetdialog.ui \
    forms/pzprinttemplate.ui \
    forms/printselectdialog.ui \
    forms/logindialog.ui \
    forms/seccondialog.ui \
    forms/searchdialog.ui \
    forms/impothmoddialog.ui \
    forms/antijzdialog.ui \
    forms/gdzcadmindialog.ui \
    forms/dtfyadmindialog.ui \
    forms/tdpreviewdialog.ui \
    forms/showtzdialog.ui \
    forms/printTemplates/printtemplatedz.ui \
    forms/printTemplates/printtemplatetz.ui \
    forms/previewdialog.ui \
    forms/printTemplates/printtemplatestat.ui \
    forms/accountpropertydialog.ui \
    forms/printTemplates/gdzcjtzjhztable.ui \
    forms/printTemplates/dtfyjttxhztable.ui \
    forms/subjectsearchform.ui \
    forms/viewpzseterrorform.ui \
    forms/pzdsform.ui \
    forms/logview.ui \
    forms/versionmanager.ui \
    forms/curstatdialog.ui \
    forms/showdzdialog2.ui \
    forms/subjectrangeselectdialog.ui \
    forms/pzdialog.ui \
    forms/apcbase.ui \
    forms/apcsuite.ui \
    forms/apcbank.ui \
    forms/apcsubject.ui \
    forms/apcreport.ui \
    forms/apclog.ui \
    forms/historypzform.ui \
    forms/subsysjoincfgform.ui \
    forms/suiteswitchpanel.ui \
    forms/databaseaccessform.ui \
    forms/apcdata.ui \
    forms/transferoutdialog.ui \
    forms/transferindialog.ui \
    forms/testform.ui \
    forms/nabaseinfodialog.ui \
    forms/newsndsubdialog.ui \
    forms/importovaccdlg.ui \
    forms/pztemplateoptionform.ui \
    forms/taxesexcelfilecfgform.ui \
    forms/taxescomparisonform.ui \
    forms/outpuexceldlg.ui \
    forms/notemgrform.ui \
    externaltoolconfigform.ui \
    forms/specsubcodecfgform.ui \
    forms/frmmessagebox.ui \
    forms/appcommcfgpanel.ui \
    forms/stationcfgform.ui \
    forms/crtaccountfromolddlg.ui \
    forms/batchoutputdialog.ui \
    forms/batchimportdialog.ui \
    forms/specsubcfgform.ui \
    forms/lookysyfitemform.ui

INCLUDEPATH +=  $$PWD/application \
                $$PWD/common

RESOURCES += \
    imgers.qrc \
    config.qrc \
    tableprinterresource.qrc \
    tableprinterresource.qrc \
    files.qrc

OTHER_FILES += \
    PrjExplain/ProjectExplain.txt \
    todos.txt \
    PrjExplain/数据库移植注意事txt \
    PrjExplain/配置变量.txt \
    PrjExplain/操作指南.txt \
    revisionHistorys \
    账户文本版本说明.txt \
    PrjExplain/任务需求分析.txt \
    bugs.txt

win32{
    RC_FILE = exeico.rc
}
