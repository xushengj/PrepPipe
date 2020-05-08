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

# Disable threading for now
DEFINES += SUPP_NO_THREADS

#if a build without GUI is needed, enable this define:
# DEFINES += SUPP_NO_GUI

SOURCES += \
    src/gui/AnonymousElementListController.cpp \
    src/gui/BreadcrumbWidget.cpp \
    src/gui/ConfigurationInputWidget.cpp \
    src/gui/DropTestLabel.cpp \
    src/gui/EditorBase.cpp \
    src/gui/ElementListWidget.cpp \
    src/gui/ExecuteOptionDialog.cpp \
    src/gui/ExecuteWindow.cpp \
    src/gui/ImportFileDialog.cpp \
    src/gui/NameListWidget.cpp \
    src/gui/NamedElementListController.cpp \
    src/gui/ObjectInputEdit.cpp \
    src/gui/ObjectTreeWidget.cpp \
    src/gui/PlainTextObjectEditor.cpp \
    src/gui/SimpleParser/BoundaryDeclarationEditWidget.cpp \
    src/gui/SimpleParser/SPContentInputWidget.cpp \
    src/gui/SimpleParser/SPMarkInputWidget.cpp \
    src/gui/SimpleParser/SPRuleInputWidget.cpp \
    src/gui/SimpleParser/SPRulePatternInputWidget.cpp \
    src/gui/SimpleParser/SimpleParserEditor.cpp \
    src/gui/SimpleParser/SimpleParserGUIObject.cpp \
    src/gui/SimpleTextGenerator/STGAliasListWidget.cpp \
    src/gui/SimpleTextGenerator/STGEditor.cpp \
    src/gui/SimpleTextGenerator/STGFragmentInputWidget.cpp \
    src/gui/SimpleTextGenerator/STGFragmentParameterReplacementEditDialog.cpp \
    src/gui/SimpleTextGenerator/SimpleTextGeneratorGUIObject.cpp \
    src/gui/TextEditor.cpp \
    src/gui/GeneralTreeEditor.cpp \
    src/gui/MIMEDataEditor.cpp \
    src/lib/DataObject/GeneralTreeObject.cpp \
    src/lib/DataObject/MIMEDataObject.cpp \
    src/lib/DataObject/PlainTextObject.cpp \
    src/lib/ExecuteObject.cpp \
    src/lib/FileBackedObject.cpp \
    src/lib/ImportedObject.cpp \
    src/lib/IntrinsicObject.cpp \
    src/lib/ObjectBase.cpp \
    src/lib/ObjectContext.cpp \
    src/lib/StaticObjectIndexDB.cpp \
    src/lib/TaskObject.cpp \
    src/lib/TaskObject/SimpleParserObject.cpp \
    src/lib/TaskObject/SimpleTextGeneratorObject.cpp \
    src/lib/TaskObject/SimpleTreeTransformObject.cpp \
    src/lib/TaskObject/SimpleWorkflowObject.cpp \
    src/lib/TaskObject/TestTaskObject.cpp \
    src/lib/Tree/Configuration.cpp \
    src/lib/Tree/SimpleParser.cpp \
    src/lib/Tree/SimpleTextGenerator.cpp \
    src/lib/Tree/SimpleTreeTransform.cpp \
    src/lib/Tree/Tree.cpp \
    src/main.cpp \
    src/gui/EditorWindow.cpp \
    src/misc/MessageLogger.cpp \
    src/misc/Settings.cpp \
    src/utils/NameSorting.cpp \
    src/utils/XMLUtilities.cpp

HEADERS += \
    src/gui/AnonymousElementListController.h \
    src/gui/BreadcrumbWidget.h \
    src/gui/ConfigurationInputWidget.h \
    src/gui/DropTestLabel.h \
    src/gui/EditorBase.h \
    src/gui/EditorWindow.h \
    src/gui/ElementListWidget.h \
    src/gui/ExecuteOptionDialog.h \
    src/gui/ExecuteWindow.h \
    src/gui/ImportFileDialog.h \
    src/gui/NameListWidget.h \
    src/gui/NamedElementListController.h \
    src/gui/ObjectInputEdit.h \
    src/gui/ObjectTreeWidget.h \
    src/gui/PlainTextObjectEditor.h \
    src/gui/SimpleParser/BoundaryDeclarationEditWidget.h \
    src/gui/SimpleParser/SPContentInputWidget.h \
    src/gui/SimpleParser/SPMarkInputWidget.h \
    src/gui/SimpleParser/SPRuleInputWidget.h \
    src/gui/SimpleParser/SPRulePatternInputWidget.h \
    src/gui/SimpleParser/SimpleParserEditor.h \
    src/gui/SimpleParser/SimpleParserGUIObject.h \
    src/gui/SimpleTextGenerator/STGAliasListWidget.h \
    src/gui/SimpleTextGenerator/STGEditor.h \
    src/gui/SimpleTextGenerator/STGFragmentInputWidget.h \
    src/gui/SimpleTextGenerator/STGFragmentParameterReplacementEditDialog.h \
    src/gui/SimpleTextGenerator/SimpleTextGeneratorGUIObject.h \
    src/gui/TextEditor.h \
    src/gui/GeneralTreeEditor.h \
    src/gui/MIMEDataEditor.h \
    src/lib/DataObject/GeneralTreeObject.h \
    src/lib/DataObject/MIMEDataObject.h \
    src/lib/DataObject/PlainTextObject.h \
    src/lib/ExecuteObject.h \
    src/lib/FileBackedObject.h \
    src/lib/ImportedObject.h \
    src/lib/IntrinsicObject.h \
    src/lib/ObjectBase.h \
    src/lib/ObjectContext.h \
    src/lib/StaticObjectIndexDB.h \
    src/lib/TaskObject.h \
    src/lib/TaskObject/SimpleParserObject.h \
    src/lib/TaskObject/SimpleTextGeneratorObject.h \
    src/lib/TaskObject/SimpleTreeTransformObject.h \
    src/lib/TaskObject/SimpleWorkflowObject.h \
    src/lib/TaskObject/TestTaskObject.h \
    src/lib/Tree/Configuration.h \
    src/lib/Tree/SimpleParser.h \
    src/lib/Tree/SimpleTextGenerator.h \
    src/lib/Tree/SimpleTreeTransform.h \
    src/lib/Tree/Tree.h \
    src/misc/MessageLogger.h \
    src/misc/Settings.h \
    src/utils/NameSorting.h \
    src/utils/XMLUtilities.h

FORMS += \
    src/gui/EditorWindow.ui \
    src/gui/ExecuteOptionDialog.ui \
    src/gui/ExecuteWindow.ui \
    src/gui/GeneralTreeEditor.ui \
    src/gui/ImportFileDialog.ui \
    src/gui/MIMEDataEditor.ui \
    src/gui/ObjectInputEdit.ui \
    src/gui/SimpleParser/BoundaryDeclarationEditWidget.ui \
    src/gui/SimpleParser/SPContentInputWidget.ui \
    src/gui/SimpleParser/SPMarkInputWidget.ui \
    src/gui/SimpleParser/SPRuleInputWidget.ui \
    src/gui/SimpleParser/SPRulePatternInputWidget.ui \
    src/gui/SimpleParser/SimpleParserEditor.ui \
    src/gui/SimpleTextGenerator/STGEditor.ui \
    src/gui/SimpleTextGenerator/STGFragmentInputWidget.ui \
    src/gui/SimpleTextGenerator/STGFragmentParameterReplacementEditDialog.ui

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
