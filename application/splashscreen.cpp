#include "splashscreen.h"
#include "global.h"

#include <QVBoxLayout>
#include <QApplication>

SplashScreen::SplashScreen(const QPixmap &pixmap, Qt::WindowFlags flag)
    :QSplashScreen(pixmap, flag)
{
    setFixedSize(pixmap.width(), pixmap.height());
    setContentsMargins(5, 0, 5, 0);
    setEnabled(false);
    showMessage(QString("%1 Prepare loading...   ").arg(appName),
              Qt::AlignRight | Qt::AlignTop, Qt::darkGray);
    setAttribute(Qt::WA_DeleteOnClose);

    QFont font = this->font();
    font.setPixelSize(12);
    setFont(font);

    splashProgress_.setObjectName("splashProgress");
    splashProgress_.setTextVisible(false);
    splashProgress_.setFixedHeight(10);
    splashProgress_.setMaximum(100);
    splashProgress_.setValue(10);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addStretch(1);
    layout->addWidget(&splashProgress_);
    setLayout(layout);
}

void SplashScreen::setProgress(int value)
{
    qApp->processEvents();
    splashProgress_.setValue(value);
    showMessage(QString("%1 Loading: %2").arg(appName).arg(value),
              Qt::AlignRight | Qt::AlignTop, Qt::darkGray);
}
