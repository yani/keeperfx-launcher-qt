#include <QApplication>
#include <QFileInfo>
#include <QGuiApplication>
#include <QProcess>
#include <QSettings>
#include <QSslConfiguration>

#include <QPalette>
#include <QStyleFactory>
#include <QFontDatabase>

using namespace Qt::StringLiterals;

#include "helper.h"
#include "launchermainwindow.h"
#include "launcheroptions.h"
#include "settings.h"
#include "translator.h"
#include "version.h"

void setDarkTheme()
{
    qApp->setStyle(QStyleFactory::create("Fusion"));
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(42, 42, 42));
    darkPalette.setColor(QPalette::AlternateBase, QColor(66, 66, 66));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(110, 110, 110));
    darkPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(100, 100, 100));
    darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(100, 100, 100));
    qApp->setPalette(darkPalette);
}

void launcherLogFileHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    // Set logfile to '<application filename>.log'
    static QFile logFile(
            QCoreApplication::applicationDirPath()
            + "/"
            + QFileInfo(QCoreApplication::applicationFilePath()).baseName()
            + ".log"
        );

    // Check if static logfile is already open
    // and open it if it isn't
    if (!logFile.isOpen()) {
        logFile.open(QIODevice::Append | QIODevice::Text);
    }

    QTextStream out(&logFile);
    QString timeStamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QString typeStr;

    switch (type) {
    case QtDebugMsg:
        typeStr = "[DEBUG]"; break;
    case QtInfoMsg:
        typeStr = "[INFO]"; break;
    case QtWarningMsg:
        typeStr = "[WARNING]"; break;
    case QtCriticalMsg:
        typeStr = "[CRITICAL]"; break;
    case QtFatalMsg:
        typeStr = "[FATAL]"; break;
    }

    out << timeStamp << " " << typeStr << ": " << msg << Qt::endl;
    out.flush(); // Ensure data is written immediately

    if (type == QtFatalMsg) {
        abort();
    }
}

int main(int argc, char *argv[])
{
    // Create the App
    QApplication app(argc, argv);
    QApplication::setApplicationName("KeeperFX Launcher");
    QApplication::setApplicationVersion(LAUNCHER_VERSION);

    // Parse launcher options
    LauncherOptions::processApp(app);

    // Check if we need to write debug logs to a logfile
    if (LauncherOptions::isSet("log-debug") == true) {
        qInstallMessageHandler(launcherLogFileHandler);
    }

    // Create an argument list of our current launcher arguments
    // We do this because we might start a new instance of the launcher
    for (int i = 1; i < argc; ++i) { // Start from 1 to skip the program name
        LauncherOptions::argumentList << QString::fromLocal8Bit(argv[i]);
    }

    // Create Qt objects for this application binary
    QFile appFile(QCoreApplication::applicationFilePath());
    QFileInfo appFileInfo(QCoreApplication::applicationFilePath());

    // Check if this launcher instance is a new launcher binary
    // We do this because when the launcher gets updated it drops a binary with "-new" in its name
    // This way we can check if we are the new instance of the launcher and overwrite the old one
    if (appFileInfo.baseName() == "keeperfx-launcher-qt-new") {
        qDebug() << "We are the \"-new\" launcher and should copy ourselves";

        // Get original binary path
#ifdef Q_OS_WINDOWS
        QString defaultAppBinString(QCoreApplication::applicationDirPath()
                                    + "/keeperfx-launcher-qt.exe");
#else
        QString defaultAppBinString(QCoreApplication::applicationDirPath()
                                    + "/keeperfx-launcher-qt");
#endif

        // Make sure original launcher is removed
        QFile defaultAppBinFile(defaultAppBinString);
        if (defaultAppBinFile.exists()) {
            defaultAppBinFile.remove();
            qDebug() << "Default launcher removed";
        }

        // Copy this updated launcher in place of the old one
        appFile.copy(defaultAppBinString);
        qDebug() << "Copied \"-new\" launcher as default launcher";

        // Start the copied launcher
        QProcess::startDetached(defaultAppBinString, LauncherOptions::argumentList);
        return 0;

    } else if (appFileInfo.baseName() == "keeperfx-launcher-qt") {
        // We are the default launcher binary
        // So we can remove the "-new" one if it exists

        // Get original binary path
#ifdef Q_OS_WINDOWS
        QString newAppBinString(QCoreApplication::applicationDirPath()
                                + "/keeperfx-launcher-qt-new.exe");
#else
        QString newAppBinString(QCoreApplication::applicationDirPath()
                                + "/keeperfx-launcher-qt-new");
#endif

        // Check if copied launcher exists and remove it
        QFile newAppBin(newAppBinString);
        if (newAppBin.exists()) {
            newAppBin.remove();
            qDebug() << "Launcher \"-new\" removed";
        }
    }

    // Detect if we need to switch from wayland to xcb (on UNIX)
    // We prefer xcb because wayland is missing a few features we'd like:
    // - position our window in the middle of the screen
    // - show the monitor number on the screen when selecting what monitor to play the game on
#ifdef Q_OS_UNIX
    if (qgetenv("QT_QPA_PLATFORM") != "xcb" && QGuiApplication::platformName() == "wayland") {
        qDebug() << "Switching from 'wayland' to 'xcb' by spawning child process";
        // Prevent child Qt application from reusing the session manager
        qunsetenv("SESSION_MANAGER");
        // Set platform to xcb
        qputenv("QT_QPA_PLATFORM", "xcb");
        // Run new process and pipe return value
        return QProcess::execute(argv[0], LauncherOptions::argumentList);
    }
#endif

    // Info: Log some stuff
    qInfo() << "Launcher Binary:" << QCoreApplication::applicationFilePath();
    qInfo() << "Launcher Directory:" << QCoreApplication::applicationDirPath();
    qInfo() << "Launcher Version:" << LAUNCHER_VERSION;

    // Info: OS
#ifdef Q_OS_WINDOWS
    qInfo() << "Launcher Build: Windows";
#else
    qInfo() << "Launcher Build: UNIX";
#endif

    // Info: Platform
    qInfo() << "Platform:" << QGuiApplication::platformName();

    // Info: Root on linux
#ifdef Q_OS_LINUX
    if (QString("root") == QString(qgetenv("USER").toLower())) {
        qInfo() << "Running as root";
    }
#endif

    // Info: Running as flatpak
    // This can be the case if you are developing in a QtCreator flatpak
    // We need this info to start the game with a different command
    // The flatpak should be given the '--filesystem=host' permission
    if (qEnvironmentVariableIsSet("FLATPAK_ID")) {
        qInfo() << "Running inside Flatpak: " << qgetenv("FLATPAK_ID");
    }

// Info: Running under Wine
#ifdef Q_OS_WINDOWS
    if (Helper::isRunningUnderWine()) {
        qInfo() << "Running under Wine";
        qInfo() << "Wine version:" << Helper::getWineVersion();
        qInfo() << "Wine host:" << Helper::getWineHostMachineName();
    }
#endif

    // Disable SSL verification
    // TODO: eventually add SSL certs
    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    QSslConfiguration::setDefaultConfiguration(sslConfig);

    // Load the launcher and kfx settings
    // Also try and copy over defaults
    Settings::load();

    // Load translator
    qDebug() << "System locale:" << QLocale::system().name();
    qDebug() << "Launcher language:" << Settings::getLauncherSetting("LAUNCHER_LANGUAGE").toString();
    Translator *translator = new Translator;
    //translator->loadPotTranslations(Settings::getLauncherSetting("LAUNCHER_LANGUAGE").toString());
    translator->loadPotTranslations("nl");
    app.installTranslator(translator);

    // Force dark theme
    setDarkTheme();

    // Create the main window and show it
    LauncherMainWindow mainWindow;
    mainWindow.show();

    // Execute main event loop
    return app.exec();
}
