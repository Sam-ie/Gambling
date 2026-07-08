#include "gambling.h"

#include <QApplication>
#include <QScreen>
#include <QPalette>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

#ifdef Q_OS_ANDROID
    // Android：强制亮色 Palette，覆盖设备暗色主题
    QPalette pal = a.palette();
    pal.setColor(QPalette::Window,          Qt::white);
    pal.setColor(QPalette::Base,            Qt::white);
    pal.setColor(QPalette::AlternateBase,   QColor(245, 245, 245));
    pal.setColor(QPalette::Text,            Qt::black);
    pal.setColor(QPalette::WindowText,      Qt::black);
    pal.setColor(QPalette::ButtonText,      Qt::black);
    pal.setColor(QPalette::Button,          QColor(240, 240, 240));
    pal.setColor(QPalette::Highlight,       QColor(200, 220, 255));
    pal.setColor(QPalette::HighlightedText, Qt::black);
    pal.setColor(QPalette::ToolTipBase,     QColor(255, 255, 220));
    pal.setColor(QPalette::ToolTipText,     Qt::black);
    a.setPalette(pal);
#endif

    // 全局滚动条/滑块/复选框样式（桌面端 + Android 通用）
    // 黑屏根因是 tabWidget 自身 stylesheet，不是这里；Android 上已单独清除 tabWidget stylesheet
    a.setStyleSheet(
        "QScrollBar:vertical { width: 14px; }"
        "QScrollBar:horizontal { height: 14px; }"
        "QScrollBar::handle:vertical, QScrollBar::handle:horizontal {"
        "  background: #aaa; border-radius: 4px; min-height: 32px; }"
        "QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }"
        "QSlider::groove:horizontal { height: 10px; background: #ddd; border-radius: 4px; }"
        "QSlider::handle:horizontal { width: 28px; height: 28px; margin: -9px 0;"
        "  background: #6a6a6a; border-radius: 14px; }"
        "QSlider::handle:horizontal:disabled { background: #bbb; }"
        "QSlider::sub-page:horizontal { background: #5294e2; border-radius: 4px; }"
        "QSlider::sub-page:horizontal:disabled { background: #ccc; }"
        "QCheckBox::indicator { width: 24px; height: 24px; }");

    Gambling w;
#ifdef Q_OS_ANDROID
    w.showFullScreen();
#else
    // 限制窗口不超过屏幕可用空间
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect avail = screen->availableGeometry();
        QSize ws = w.size();
        w.resize(qMin(ws.width(), avail.width()), qMin(ws.height(), avail.height()));
    }
    w.show();
#endif
    return a.exec();
}
