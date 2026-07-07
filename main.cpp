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

    // 全局滚动条加宽，触摸友好
    a.setStyleSheet(
        "QScrollBar:vertical { width: 14px; }"
        "QScrollBar:horizontal { height: 14px; }"
        "QScrollBar::handle:vertical, QScrollBar::handle:horizontal {"
        "  background: #aaa; border-radius: 4px; min-height: 32px; }"
        "QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }"
        // 滑块加粗加高，触摸友好
        "QSlider::groove:horizontal { height: 10px; background: #ddd; border-radius: 4px; }"
        "QSlider::handle:horizontal { width: 28px; height: 28px; margin: -9px 0;"
        "  background: #6a6a6a; border-radius: 14px; }"
        "QSlider::handle:horizontal:disabled { background: #bbb; }"
        "QSlider::sub-page:horizontal { background: #5294e2; border-radius: 4px; }"
        "QSlider::sub-page:horizontal:disabled { background: #ccc; }"
        // 复选框加大
        "QCheckBox::indicator { width: 24px; height: 24px; }");

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
