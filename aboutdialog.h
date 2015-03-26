#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include "padialog.h"
#include "VersionRev.h"


class AboutDialog : public PaDialog
{
public:
    AboutDialog(QWidget *parent = 0);
    ~AboutDialog();
};

#endif // ABOUTDIALOG_H
