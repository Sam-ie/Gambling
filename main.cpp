#include "gambling.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

#ifdef Q_OS_ANDROID
#include <QScreen>
#endif

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "Gambling_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    Gambling w;
#ifdef Q_OS_ANDROID
    w.showFullScreen();
#else
    w.show();
#endif
    return a.exec();
}
