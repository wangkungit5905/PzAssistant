#-------------------------------------------------
#
# Project created by QtCreator 2011-01-19T12:59:18
#
#-------------------------------------------------

QT       += core widgets sql xml printsupport axcontainer

TARGET = PzAssistant
DESTDIR = $${PWD}/../workDir/
TEMPLATE = app

#CONFIG += qaxcontainer

SOURCES += main.cpp\
    connection.cpp \
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
    aboutform.cpp \
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
    widgets/configpanels.cpp \
    excel/ExcelUtil.cpp \
    taxescomparisonform.cpp \
    outputexceldlg.cpp

HEADERS  += \
    connection.h \
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
    aboutform.h \
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
    widgets/configpanels.h \
    excel/ExcelUtil.h \
    taxescomparisonform.h \
    outputexceldlg.h

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
    forms/aboutform.ui \
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
    forms/outpuexceldlg.ui

RESOURCES += \
    imgers.qrc \
    config.qrc \
    tableprinterresource.qrc \
    tableprinterresource.qrc

OTHER_FILES += \
    PrjExplain/ProjectExplain.txt \
    todos.txt \
    PrjExplain/数据库移植注意事txt \
    PrjExplain/配置变量.txt \
    PrjExplain/操作指南.txt \
    PrjExplain/revisionHistorys \
    账户文本版本说明.txt \
    ini/revisionHistorys.ini \
    PrjExplain/任务需求分�?.txt \
    bugs.txt \
    修改日志.txt

win32{
    RC_FILE = exeico.rc
}
