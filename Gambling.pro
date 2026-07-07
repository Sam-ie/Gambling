QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

INCLUDEPATH += core core/npc ui

SOURCES += \
    core/gameengine.cpp \
    core/historyrecord.cpp \
    core/npc/npcbase.cpp \
    core/npc/npcfactory.cpp \
    ui/gambling.cpp \
    ui/npccirclewidget.cpp \
    ui/numberpicker.cpp \
    main.cpp

HEADERS += \
    core/gameengine.h \
    core/historyrecord.h \
    core/npc/npcbase.h \
    core/npc/npcfactory.h \
    core/npc/honestnpc.h \
    core/npc/deceptivenpc.h \
    core/npc/swingernpc.h \
    core/npc/repeaternpc.h \
    core/npc/forgivingnpc.h \
    core/npc/reinforcementnpc.h \
    core/npc/grudgernpc.h \
    core/npc/detectivenpc.h \
    core/npc/pavlovnpc.h \
    core/npc/majoritynpc.h \
    core/npc/periodicnpc.h \
    ui/gambling.h \
    ui/npccirclewidget.h \
    ui/numberpicker.h

FORMS += \
    ui/gambling.ui \
    ui/welcome.ui

TRANSLATIONS += \
    Gambling_en_US.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

android {
    ANDROID_APP_SCREEN_SUPPORT = anyDensity|large|normal|small
    ANDROID_APP_ORIENTATION = landscape
    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

    ANDROID_SIGN_STORE      = $$(QT_ANDROID_KEYSTORE_PATH)
    ANDROID_SIGN_STORE_PASS = $$(QT_ANDROID_KEYSTORE_STORE_PASS)
    ANDROID_SIGN_ALIAS      = $$(QT_ANDROID_KEYSTORE_ALIAS)
    ANDROID_SIGN_ALIAS_PASS = $$(QT_ANDROID_KEYSTORE_KEY_PASS)
}
