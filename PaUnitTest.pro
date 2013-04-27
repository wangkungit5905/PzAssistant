#-------------------------------------------------
#
# Project created by QtCreator 2013-04-16T10:10:53
#
#-------------------------------------------------

QT       += sql xml testlib

TARGET = TestPzA
include(testmain/config.pri)
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += \ 
    widgets.cpp \
    viewpzseterrorform.cpp \
    version.cpp \
    utils.cpp \
    tem.cpp \
    tdpreviewdialog.cpp \
    tables.cpp \
    subjectsearchform.cpp \
    subjectConfigDialog.cpp \
    subject.cpp \
    statutil.cpp \
    sqltooldialog.cpp \
    setupbasedialog2.cpp \
    securitys.cpp \
    ratesetdialog.cpp \
    PzSet.cpp \
    pzdsform.cpp \
    pzdialog2.cpp \
    pz.cpp \
    printUtils.cpp \
    printetemplate.cpp \
    previewdialog.cpp \
    otherModule.cpp \
    mainwindow.cpp \
    jzhdsyinfoinputdlg.cpp \
    HierarchicalHeaderView.cpp \
    global.cpp \
    excelUtils.cpp \
    ExcelFormat.cpp \
    dialogs.cpp \
    dialog3.cpp \
    dialog2.cpp \
    delegates2.cpp \
    dbutil.cpp \
    connection.cpp \
    config.cpp \
    completsubinfodialog.cpp \
    cal.cpp \
    BasicExcel.cpp \
    appmodel.cpp \
    account.cpp \
    aboutform.cpp \
    logs/logview.cpp \
    logs/Logger.cpp \
    logs/FileAppender.cpp \
    logs/ConsoleAppender.cpp \
    logs/AbstractStringAppender.cpp \
    logs/AbstractAppender.cpp \
    testmain/main.cpp \
    testmain/tst_patest.cpp \
    testmain/testpzsetstat.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"

OTHER_FILES += \
    todos.txt \
    README.md \
    qt.conf \
    exeico.rc \
    icons/16x16/actions/viewzoomout.png \
    icons/16x16/actions/viewzoomin.png \
    icons/16x16/actions/preview.png \
    icons/16x16/actions/ok.png \
    icons/16x16/actions/fileprint.png \
    icons/16x16/actions/fileopen.png \
    icons/16x16/actions/configure.png \
    icons/16x16/actions/button_cancel.png \
    icons/16x16/actions/1rightarrow.png \
    icons/16x16/actions/1leftarrow.png \
    images/verify-ok.png \
    images/verify-not.png \
    images/Thumbs.db \
    images/tag.png \
    images/sortByZbNum.PNG \
    images/sortByPzNum.PNG \
    images/selAccs.png \
    images/save.png \
    images/pzs_open.png \
    images/pzs_edit.png \
    images/pz_search.png \
    images/pzs_close.png \
    images/pz_save.png \
    images/pz-ok.png \
    images/pz_del.png \
    images/pz_close.png \
    images/pz-cancel.png \
    images/pz_add.png \
    images/printer_remove.PNG \
    images/printer_add.PNG \
    images/printer.png \
    images/paste.png \
    images/open.png \
    images/not_tag.png \
    images/newPingzh.png \
    images/newAccs.png \
    images/naviTo.png \
    images/jz.png \
    images/go-up.png \
    images/go_previous.png \
    images/go_next.png \
    images/go_last.png \
    images/go_first.png \
    images/go-down.png \
    images/freespeak.png \
    images/delPingzh.png \
    images/app-orca.png \
    images/antijz.png \
    images/action_del.png \
    images/action_add.png \
    images/acc_open.png \
    images/acc_new.png \
    images/acc_import.png \
    images/acc_export.png \
    images/acc_close.png \
    images/6.png \
    images/5.png \
    images/4.png \
    images/3.png \
    images/2.png \
    images/1.png \
    ini/revisionHistorys.ini

HEADERS += \
    widgets.h \
    viewpzseterrorform.h \
    version.h \
    utils.h \
    tem.h \
    tdpreviewdialog.h \
    tables.h \
    subjectsearchform.h \
    subjectConfigDialog.h \
    subject.h \
    statutil.h \
    sqltooldialog.h \
    setupbasedialog2.h \
    securitys.h \
    seccondialog-.h \
    ratesetdialog.h \
    PzSet.h \
    pzdsform.h \
    pzdialog2.h \
    pz.h \
    printUtils.h \
    printtemplate.h \
    previewdialog.h \
    otherModule.h \
    mainwindow.h \
    jzhdsyinfoinputdlg.h \
    HierarchicalHeaderView.h \
    global.h \
    excelUtils.h \
    ExcelFormat.h \
    dialogs.h \
    dialog3.h \
    dialog2.h \
    delegates2.h \
    dbutil.h \
    connection.h \
    config.h \
    completsubinfodialog.h \
    common.h \
    commdatastruct.h \
    cal.h \
    c.h \
    BasicExcel.h \
    appmodel.h \
    account.h \
    aboutform.h \
    logs/logview.h \
    logs/LogStruct.h \
    logs/Logger.h \
    logs/FileAppender.h \
    logs/ConsoleAppender.h \
    logs/AbstractStringAppender.h \
    logs/AbstractAppender.h \
    testmain/tst_patest.h \
    testmain/testpzsetstat.h

RESOURCES += \
    tableprinterresource.qrc \
    imgers.qrc \
    config.qrc

FORMS += \
    forms/viewpzseterrorform.ui \
    forms/viewextradialog.ui \
    forms/versionmanager.ui \
    forms/tdtableprinter.ui \
    forms/tdpreviewdialog.ui \
    forms/subjectsearchform.ui \
    forms/subjectextradialog.ui \
    forms/sqltooldialog.ui \
    forms/sndsubconfig.ui \
    forms/showtzdialog.ui \
    forms/showdzdialog.ui \
    forms/setupbasedialog2.ui \
    forms/setupbasedialog.ui \
    forms/setupbankdialog.ui \
    forms/seccondialog.ui \
    forms/searchdialog.ui \
    forms/reportdialog.ui \
    forms/ratesetdialog.ui \
    forms/pzprinttemplate.ui \
    forms/pzdsform.ui \
    forms/pzdialog2.ui \
    forms/printselectdialog.ui \
    forms/previewdialog.ui \
    forms/openpzdialog.ui \
    forms/openaccountdialog.ui \
    forms/mainwindow.ui \
    forms/lookupsubjectextradialog.ui \
    forms/logview.ui \
    forms/logindialog.ui \
    forms/jzhdsyinfoinputdlg.ui \
    forms/impothmoddialog.ui \
    forms/historypzdialog.ui \
    forms/happensubseldialog.ui \
    forms/gdzcadmindialog.ui \
    forms/dtfyadmindialog.ui \
    forms/detailsviewdialog2.ui \
    forms/detailsviewdialog.ui \
    forms/detailextradialog.ui \
    forms/createaccountdialog.ui \
    forms/completsubinfodialog.ui \
    forms/collectpzdialog.ui \
    forms/basummaryform.ui \
    forms/basicdatadialog.ui \
    forms/basedataeditdialog.ui \
    forms/antijzdialog.ui \
    forms/accountpropertydialog.ui \
    forms/aboutform.ui \
    forms/printTemplates/printtemplatetz.ui \
    forms/printTemplates/printtemplatestat.ui \
    forms/printTemplates/printtemplatedz.ui \
    forms/printTemplates/gdzcjtzjhztable.ui \
    forms/printTemplates/dtfyjttxhztable.ui


