#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include "padialog.h"

//#define REVISION_NUM    "0"
//#define REVISION_STR    "0"
#define STRDATE           "2015-03-23\0"
#define STRPRODUCTVER     "1.0.23\0"

//#define VCS_REVISION      "git-87-5c3a069"

//VERSION_REV = $$system(git rev-list --count HEAD)               87
//count(VERSION_REV, 1) {
//VERSION_REV = git-$$VERSION_REV-$$system(git rev-parse --short HEAD)   5c3a069

class AboutDialog : public PaDialog
{
public:
    AboutDialog(QWidget *parent = 0);
    ~AboutDialog();
};

#endif // ABOUTDIALOG_H
