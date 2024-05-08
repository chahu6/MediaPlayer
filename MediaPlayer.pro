QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    opengl_widget.cpp \
    slider_pts.cpp \
    main.cpp \
    widget.cpp

HEADERS += \
    opengl_widget.h \
    slider_pts.h \
    widget.h

FORMS += \
    widget.ui

include(3rdparty/3rdparty.pri)
include(Player/Player.pri)
include(Utils/Utils.pri)

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# ffmpeg
INCLUDEPATH += $$PWD/3rdparty/ffmpeg-64bit/include
win32: LIBS += -L$$PWD/3rdparty/ffmpeg-64bit/lib -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lpostproc -lswresample -lswscale

# SLD2
INCLUDEPATH += $$PWD/3rdparty/SDL2-2.26.5/include
win32: LIBS += -L$$PWD/3rdparty/SDL2-2.26.5/lib/x64 -lSDL2 #-lSDL2main # -lSDL2test 这个库应该不用吧
unix: LIBS += -L$$PWD/3rdparty/SDL2-2.26.5/lib/x64 -lSDL2
