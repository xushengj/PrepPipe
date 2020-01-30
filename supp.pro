QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# embed translation files inside application binary
CONFIG += lrelease embed_translations
# translation files will be embedded under :/translations/ directory
QM_FILES_RESOURCE_PREFIX = translations

# set version info
VERSION = 0.0.0.0

# always keep track of log context
DEFINES += QT_MESSAGELOGCONTEXT

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/gui/BreadcrumbWidget.cpp \
    src/gui/TextEditor.cpp \
    src/gui/GeneralTreeEditor.cpp \
    src/gui/MIMEDataEditor.cpp \
    src/lib/DataObject/GeneralTreeObject.cpp \
    src/lib/DataObject/MIMEDataObject.cpp \
    src/lib/ExecuteObject.cpp \
    src/lib/IntrinsicObject.cpp \
    src/lib/ObjectBase.cpp \
    src/lib/ObjectContext.cpp \
    src/lib/StaticObjectIndexDB.cpp \
    src/lib/TaskObject.cpp \
    src/lib/TaskObject/SimpleTreeTransformObject.cpp \
    src/lib/Tree/Tree.cpp \
    src/lib/Tree/TreeConstraint.cpp \
    src/main.cpp \
    src/gui/EditorWindow.cpp \
    src/misc/MessageLogger.cpp \
    src/misc/Settings.cpp

HEADERS += \
    src/gui/BreadcrumbWidget.h \
    src/gui/EditorWindow.h \
    src/gui/TextEditor.h \
    src/gui/GeneralTreeEditor.h \
    src/gui/MIMEDataEditor.h \
    src/lib/DataObject/GeneralTreeObject.h \
    src/lib/DataObject/MIMEDataObject.h \
    src/lib/ExecuteObject.h \
    src/lib/IntrinsicObject.h \
    src/lib/ObjectBase.h \
    src/lib/ObjectContext.h \
    src/lib/StaticObjectIndexDB.h \
    src/lib/TaskObject.h \
    src/lib/TaskObject/SimpleTreeTransformObject.h \
    src/lib/Tree/SimpleTreeTransform.h \
    src/lib/Tree/Tree.h \
    src/lib/Tree/TreeConstraint.h \
    src/misc/MessageLogger.h \
    src/misc/Settings.h

FORMS += \
    src/gui/EditorWindow.ui \
    src/gui/GeneralTreeEditor.ui \
    src/gui/MIMEDataEditor.ui

TRANSLATIONS += \
    translations/supp_zh_CN.ts

# Platform dependent sources
win32 {
    SOURCES += src/misc/MessageLogger_Windows.cpp
    LIBS += -lDbgHelp
} else: unix {
    SOURCES += src/misc/MessageLogger_POSIX.cpp
    QMAKE_LFLAGS += -rdynamic
} else {
    SOURCES += src/misc/MessageLogger_OtherPlatform.cpp
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources/resources.qrc
