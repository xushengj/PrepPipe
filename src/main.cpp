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

    QApplication::setOrganizationName("SUPP Development Team");
    QApplication::setApplicationName("supp");

    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication a(argc, argv);

    Settings::createInstance();
    StaticObjectIndexDB::createInstance();

    QTranslator translator;
    QLocale defaultLocale = QLocale::system();
    if (defaultLocale.language() != QLocale::English) {
        qDebug() << "Trying to install translation for locale" << defaultLocale.name();
        if (!translator.load(defaultLocale, "supp_", "", ":/translations/")) {
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

    QCommandLineOption rootDirOpt({"r", "root"},
                               QCoreApplication::translate("main", "Set object root directory"),
                               QCoreApplication::translate("main", "directory"));
    parser.addOption(rootDirOpt);

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
            if (!translator.load(newLocale, "supp_", "", ":/translations/")) {
                qDebug() << "Failed to load translation for locale" << newLocale.name();
            } else {
                QCoreApplication::installTranslator(&translator);
            }
        }
    }
    NameSorting::init();

    QString rootDirectory;
    if (parser.isSet(rootDirOpt)) {
        rootDirectory = parser.value(rootDirOpt);
    } else {
        rootDirectory = QDir::currentPath();
    }

    QString startTask;
    if (parser.isSet(taskOpt)) {
        startTask = parser.value(taskOpt);
    }

    EditorWindow w(rootDirectory, startTask, parser.positionalArguments());
    MessageLogger::inst()->bootstrapFinished(&w);
    w.show();

    int retVal = a.exec();

    Settings::destructInstance();
    StaticObjectIndexDB::destructInstance();
    MessageLogger::destructInstance();
    return retVal;
}

