#include "src/gui/EditorWindow.h"
#include "src/lib/ObjectContext.h"
#include "src/lib/StaticObjectIndexDB.h"
#include "src/misc/Settings.h"
#include "src/misc/MessageLogger.h"
#include "src/utils/NameSorting.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QTranslator>

int main(int argc, char *argv[])
{
    MessageLogger::createInstance();

    QApplication::setOrganizationName("PrepPipe Development Team");
    QApplication::setApplicationName("PrepPipe");

    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication a(argc, argv);

    Settings::createInstance();
    StaticObjectIndexDB::createInstance();

    QTranslator translator;
    QLocale defaultLocale = QLocale::system();
    if (defaultLocale.language() != QLocale::English) {
        qDebug() << "Trying to install translation for locale" << defaultLocale.name();
        if (!translator.load(defaultLocale, "tr_", "", ":/translations/")) {
            qDebug() << "Failed to load translation for locale" << defaultLocale.name();
        } else {
            QCoreApplication::installTranslator(&translator);
        }
    }

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main", "Data processing utility program"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption localeOpt("locale",
                                 QCoreApplication::translate("main", "Set locale for translation"),
                                 QCoreApplication::translate("main", "locale"));
    parser.addOption(localeOpt);

    QCommandLineOption taskOpt({"t", "task"},
                              QCoreApplication::translate("main", "Run given task upon start"),
                              QCoreApplication::translate("main", "task"));
    parser.addOption(taskOpt);

    QCommandLineOption startDirOpt({"d", "dir"},
                               QCoreApplication::translate("main", "Override start directory"),
                               QCoreApplication::translate("main", "directory"));
    parser.addOption(startDirOpt);

    parser.addPositionalArgument(QCoreApplication::translate("main", "path"),
                                 QCoreApplication::translate("main", "The directory or file to open"));


    parser.process(a);

    if (parser.isSet(localeOpt)) {
        QString localeStr = parser.value(localeOpt);
        QLocale newLocale(localeStr);
        QLocale::setDefault(newLocale);

        // remove the translator first, in case another translation file is already loaded
        if (!translator.isEmpty()) {
            QCoreApplication::removeTranslator(&translator);
        }

        if (newLocale == QLocale::c()) {
            // this usually means the locale string is invalid
            qDebug() << "Cannot use locale" << localeStr;
        } else if (newLocale != defaultLocale) {
            qDebug() << "Trying to install translation for locale" << newLocale.name();
            if (!translator.load(newLocale, "tr_", "", ":/translations/")) {
                qDebug() << "Failed to load translation for locale" << newLocale.name();
            } else {
                QCoreApplication::installTranslator(&translator);
            }
        }
    }
    NameSorting::init();

    QString startDirectory;
    if (parser.isSet(startDirOpt)) {
        startDirectory = parser.value(startDirOpt);
    }

    QString startTask;
    if (parser.isSet(taskOpt)) {
        startTask = parser.value(taskOpt);
    }

    QString startPath;
    QStringList presets;
    if (!parser.positionalArguments().empty()) {
        QStringList posArgs = parser.positionalArguments();
        startPath = posArgs.front();
        presets = posArgs;
        presets.pop_front();
    }

    // special handling if startPath is a directory instead of a file
    // if rootDirectory is empty, then rootDirectory is set to startPath
    QFileInfo startPathInfo(startPath);
    if (startPathInfo.isDir()) {
        if (startDirectory.isEmpty()) {
            startDirectory = startPath;
        }
        startPath.clear();
    } else if (startPathInfo.isFile()) {
        if (startDirectory.isEmpty()) {
            startDirectory = startPathInfo.dir().absolutePath();
        }
    }
    qDebug() << "Start configurations: \n"
             << "\tStart directory: " << startDirectory
             << "\n\tStart task: " << startTask
             << "\n\tStart file path: " << startPath
             << "\n\tPresets: " << presets;

    EditorWindow w(startDirectory, startTask, startPath, presets);
    MessageLogger::inst()->bootstrapFinished(&w);
    w.show();

    int retVal = a.exec();

    Settings::destructInstance();
    StaticObjectIndexDB::destructInstance();
    MessageLogger::destructInstance();
    return retVal;
}

