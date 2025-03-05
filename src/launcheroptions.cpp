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

    // Define options
    // clang-format off
    QVector<QCommandLineOption> options = {
        {"log-debug", "Log the debug output of the launcher to a file"},
        {"skip-verify", "Skip the certificate verification process"},
        {"install", "Start the KeeperFX install procedure"}
    };
    // clang-format on

    // Add options to the parser
    for (const auto &option : options) {
        parser.addOption(option);
    }

    // Process
    parser.process(app);
}
