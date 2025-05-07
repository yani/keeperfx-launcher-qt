#include "launcheroptions.h"

QCommandLineParser LauncherOptions::parser;

bool LauncherOptions::isSet(const QString option)
{
    return LauncherOptions::parser.isSet(option);
}

QString LauncherOptions::getValue(const QString option)
{
    return LauncherOptions::parser.value(option);
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

        // Toggle options
        {"log-debug",                   "Log the debug output of the launcher to a file"},
        {"skip-verify",                 "Skip the certificate verification process"},
        {"install",                     "Start the KeeperFX install procedure"},
        {"skip-launcher-update",        "Do not update the launcher itself"},
        {"log-missing-translations",    "Log missing translations to debug"},
        {"no-image-cache",              "Bypass image caching"},

        // Parameters
        {"api-endpoint",                "Specify the API endpoint",                    "url"},
        {"translation-file",            "Force a PO translation file to be loaded",    "filepath"},
    };
    // clang-format on

    // Add options to the parser
    for (const auto &option : options) {
        parser.addOption(option);
    }

    // Process
    parser.process(app);
}
