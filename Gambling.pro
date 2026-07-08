QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

INCLUDEPATH += core core/npc ui core/evolution core/elimination

SOURCES += \
    core/gameengine.cpp \
    core/historyrecord.cpp \
    core/interactionrunner.cpp \
    core/npc/npcbase.cpp \
    core/npc/npcfactory.cpp \
    core/elimination/eliminationmanager.cpp \
    core/evolution/evoidle.cpp \
    core/evolution/evopairbypair.cpp \
    core/evolution/evofast.cpp \
    core/evolution/evosingleround.cpp \
    core/evolution/autoevolutionengine.cpp \
    ui/gambling.cpp \
    ui/npccirclewidget.cpp \
    ui/numberpicker.cpp \
    ui/spingroupcontroller.cpp \
    ui/helptextbuilder.cpp \
    ui/presetmanager.cpp \
    main.cpp

HEADERS += \
    core/gameengine.h \
    core/historyrecord.h \
    core/npcscoreinfo.h \
    core/payoffmatrix.h \
    core/interactionrunner.h \
    core/npc/npcbase.h \
    core/npc/npcfactory.h \
    core/npc/npcconfig.h \
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
    core/elimination/eliminationmanager.h \
    core/evolution/evolutionstate.h \
    core/evolution/evoidle.h \
    core/evolution/evopairbypair.h \
    core/evolution/evofast.h \
    core/evolution/evosingleround.h \
    core/evolution/autoevolutionengine.h \
    ui/gambling.h \
    ui/npccirclewidget.h \
    ui/numberpicker.h \
    ui/spingroupcontroller.h \
    ui/helptextbuilder.h \
    ui/presetmanager.h

FORMS += \
    ui/gambling.ui \
    ui/welcome.ui

TRANSLATIONS += Gambling_en.ts

CONFIG += lrelease
CONFIG += embed_translations

RESOURCES += i18n.qrc

# Windows 应用图标（从 Android mipmap PNG 自动生成 .ico）
win32 {
    ICON_SRC = $$PWD/android/res/mipmap-xxxhdpi/ic_launcher.png
    !exists($$PWD/app.ico): system(python -c \"from PIL import Image; img=Image.open(r'$$replace(ICON_SRC,/,\\)'); img.save(r'$$replace(PWD,/,\\)\\\\app.ico',format='ICO',sizes=[(256,256),(128,128),(64,64),(48,48),(32,32),(16,16)])\")
    RC_ICONS = app.ico
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

android {
    ANDROID_APP_SCREEN_SUPPORT = anyDensity|large|normal|small
    ANDROID_APP_ORIENTATION = landscape
    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

    # make install → 部署 .so 到 android-build/libs，供 androiddeployqt 打包 APK
    target.path = /libs/$${ANDROID_TARGET_ARCH}
    INSTALLS += target

    ANDROID_SIGN_STORE      = $$(QT_ANDROID_KEYSTORE_PATH)
    ANDROID_SIGN_STORE_PASS = $$(QT_ANDROID_KEYSTORE_STORE_PASS)
    ANDROID_SIGN_ALIAS      = $$(QT_ANDROID_KEYSTORE_ALIAS)
    ANDROID_SIGN_ALIAS_PASS = $$(QT_ANDROID_KEYSTORE_KEY_PASS)
}
