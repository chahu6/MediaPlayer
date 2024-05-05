HEADERS += \
    $$PWD/ComMessageBox.h \
    $$PWD/FpsControl.h \
    $$PWD/SingletonUtils.h \
    $$PWD/Utils.h \
    $$PWD/ComLineWidget.h \
    $$PWD/constant.h

SOURCES += \
    $$PWD/ComMessageBox.cpp \
    $$PWD/ComLineWidget.cpp \
    $$PWD/FpsControl.cpp \
    $$PWD/SingletonUtils.cpp

INCLUDEPATH += Utils # 包含Utils目录，以后就不用在头文件加Utils/了
