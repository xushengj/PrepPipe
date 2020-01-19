#include "src/gui/EditorWindow.h"
#include "src/lib/ObjectContext.h"
#include "src/misc/Settings.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QTranslator>

namespace {

void firstStageMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    const char *file = context.file ? context.file : "";
    const char *function = context.function ? context.function : "";
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtInfoMsg:
        fprintf(stderr, "Info: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    }
}

} // end of anonymous namespace

int main(int argc, char *argv[])
{
    // after a window is created, the message handler will also be replaced
    qInstallMessageHandler(firstStageMessageHandler);

    QApplication::setOrganizationName("SUPP Development Team");
    QApplication::setApplicationName("supp");

    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication a(argc, argv);

    Settings::createInstance();

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

    QCommandLineOption runOpt({"r", "run"},
                              QCoreApplication::translate("main", "Run given script and exit"),
                              QCoreApplication::translate("main", "script"));
    parser.addOption(runOpt);

    QCommandLineOption editOpt({"e", "edit"},
                               QCoreApplication::translate("main", "Set root directory of editor bundle"),
                               QCoreApplication::translate("main", "directory"));
    parser.addOption(editOpt);

    parser.process(a);

    if (parser.isSet(localeOpt)) {
        QString localeStr = parser.value(localeOpt);
        QLocale newLocale(localeStr);

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

    QString editRootDirectory;
    if (parser.isSet(editOpt)) {
        editRootDirectory = parser.value(editOpt);
    } else {
        editRootDirectory = QDir::currentPath();
    }

    EditorWindow w(new ObjectContext(editRootDirectory));
    w.show();

    int retVal = a.exec();

    Settings::destructInstance();
    return retVal;
}

