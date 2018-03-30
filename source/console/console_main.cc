//
// PROJECT:         Aspia
// FILE:            console/console_main.cc
// LICENSE:         GNU Lesser General Public License 2.1
// PROGRAMMERS:     Dmitry Chapyshev (dmitry@aspia.ru)
//

#include "console/console_main.h"

#include <QCommandLineParser>

#include "base/logging.h"
#include "console/console_window.h"
#include "version.h"

namespace aspia {

int ConsoleMain(int argc, char *argv[])
{
    LoggingSettings settings;
    settings.logging_dest = LOG_TO_ALL;
    settings.lock_log = LOCK_LOG_FILE;

    InitLogging(settings);

    QApplication application(argc, argv);
    application.setOrganizationName(QStringLiteral("Aspia"));
    application.setApplicationName(QStringLiteral("Console"));
    application.setApplicationVersion(QStringLiteral(ASPIA_VERSION_STRING));

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::tr("Aspia Console"));
    parser.addHelpOption();
    parser.addPositionalArgument(QStringLiteral("file"), QCoreApplication::tr("The file to open."));
    parser.process(application);

    QStringList arguments = parser.positionalArguments();

    QString file_path;
    if (arguments.size())
        file_path = arguments.front();

    ConsoleWindow window(file_path);
    window.show();
    window.activateWindow();

    int ret = application.exec();

    ShutdownLogging();
    return ret;
}

} // namespace aspia
