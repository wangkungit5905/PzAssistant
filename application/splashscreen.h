#ifndef SPLASHSCREEN_H
#define SPLASHSCREEN_H

#include <QSplashScreen>
#include <QProgressBar>

class SplashScreen : public QSplashScreen
{
  Q_OBJECT
public:
  explicit SplashScreen(const QPixmap &pixmap = QPixmap(), Qt::WindowFlags flag = 0);

  void setProgress(int value);

private:
  QProgressBar splashProgress_;

};

#endif // SPLASHSCREEN_H
