#-------------------------------------------------
#
# Project created by QtCreator 2011-01-19T12:59:18
#
#-------------------------------------------------

QT       += core gui sql xml

TARGET = PzAssistant
TEMPLATE = app

#CONFIG += qaxcontainer

SOURCES += main.cpp\
    connection.cpp \
    config.cpp \
    delegates.cpp \
    appmodel.cpp \
    subjectConfigDialog.cpp \
    dialogs.cpp \
    utils.cpp \
    widgets.cpp \
    sqltooldialog.cpp \
    completsubinfodialog.cpp \
    setupbasedialog.cpp \
    dialog2.cpp \
    mainwindow.cpp \
    ExcelFormat.cpp \
    BasicExcel.cpp \
    printUtils.cpp \
    printetemplate.cpp \
    securitys.cpp \
    global.cpp \
    pzdialog2.cpp \
    delegates2.cpp \
    tdpreviewdialog.cpp \
    HierarchicalHeaderView.cpp \
    previewdialog.cpp \
    account.cpp \
    otherModule.cpp \
    commdatastruct.cpp \
    dialog3.cpp \
    pz.cpp \
    subjectmanager.cpp \
    tables.cpp \
    cal.cpp \
    tem.cpp \
    subjectsearchform.cpp \
    viewpzseterrorform.cpp \
    aboutform.cpp \
    jzhdsyinfoinputdlg.cpp \
    pzdsform.cpp

HEADERS  += \
    connection.h \
    config.h \
    global.h \
    delegates.h \
    appmodel.h \
    common.h \
    subjectConfigDialog.h \
    dialogs.h \
    c.h \
    utils.h \
    widgets.h \
    forms/sqltooldialog.h \
    sqltooldialog.h \
    completsubinfodialog.h \
    setupbasedialog.h \
    dialog2.h \
    mainwindow.h \
    ExcelFormat.h \
    BasicExcel.h \
    printUtils.h \
    printtemplate.h \
    commdatastruct.h \
    securitys.h \
    pzdialog2.h \
    delegates2.h \
    dialog3.h \
    tdpreviewdialog.h \
    HierarchicalHeaderView.h \
    previewdialog.h \
    account.h \
    otherModule.h \
    pz.h \
    subjectmanager.h \
    tables.h \
    cal.h \
    tem.h \
    subjectsearchform.h \
    viewpzseterrorform.h \
    aboutform.h \
    jzhdsyinfoinputdlg.h \
    pzdsform.h

FORMS    += \
    forms/createaccountdialog.ui \
    forms/openaccountdialog.ui \
    forms/openpzdialog.ui \
    forms/collectpzdialog.ui \
    forms/sndsubconfig.ui \
    forms/basicdatadialog.ui \
    forms/subjectextradialog.ui \
    forms/reportdialog.ui \
    forms/sqltooldialog.ui \
    forms/completsubinfodialog.ui \
    forms/setupbasedialog.ui \
    forms/setupbankdialog.ui \
    forms/detailsviewdialog.ui \
    forms/detailextradialog.ui \
    forms/basummaryform.ui \
    forms/mainwindow.ui \
    forms/ratesetdialog.ui \
    forms/viewextradialog.ui \
    forms/detailsviewdialog2.ui \
    forms/pzprinttemplate.ui \
    forms/printselectdialog.ui \
    forms/setupbasedialog2.ui \
    forms/logindialog.ui \
    forms/seccondialog.ui \
    forms/pzdialog2.ui \
    forms/searchdialog.ui \
    forms/impothmoddialog.ui \
    forms/antijzdialog.ui \
    forms/basedataeditdialog.ui \
    forms/gdzcadmindialog.ui \
    forms/dtfyadmindialog.ui \
    forms/happensubseldialog.ui \
    forms/tdpreviewdialog.ui \
    forms/showtzdialog.ui \
    forms/showdzdialog.ui \
    forms/historypzdialog.ui \
    forms/printTemplates/printtemplatedz.ui \
    forms/printTemplates/printtemplatetz.ui \
    forms/previewdialog.ui \
    forms/printTemplates/printtemplatestat.ui \
    forms/lookupsubjectextradialog.ui \
    forms/accountpropertydialog.ui \
    forms/printTemplates/gdzcjtzjhztable.ui \
    forms/pzdialog3.ui \
    forms/printTemplates/dtfyjttxhztable.ui \
    forms/subjectsearchform.ui \
    forms/viewpzseterrorform.ui \
    forms/aboutform.ui \
    forms/jzhdsyinfoinputdlg.ui \
    forms/pzdsform.ui

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
    ini/revisionHistorys.ini

win32{
    RC_FILE = exeico.rc
}





































































































