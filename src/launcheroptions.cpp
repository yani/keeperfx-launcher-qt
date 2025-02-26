#include "launcheroptions.h"

QCommandLineParser LauncherOptions::parser;

bool LauncherOptions::isSet(const QString option)
{
    return LauncherOptions::parser.isSet(option);
}

void LauncherOptions::processApp(QApplication &app)
{
    // Set up the command line parser
    LauncherOptions::parser.setApplicationDescription("KeeperFX Launcher");
    LauncherOptions::parser.addHelpOption();
    LauncherOptions::parser.addVersionOption();

    // --log-debug option
    QCommandLineOption logDebugOption("log-debug", "Log the debug output of the launcher to a file");
    parser.addOption(logDebugOption);

    // --skip-verify option
    QCommandLineOption skipVerifyOption("skip-verify", "Skip the certificate verification process");
    parser.addOption(skipVerifyOption);

    // Process
    parser.process(app);
}
