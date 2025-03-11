#include <QApplication>
#include <QGuiApplication>
#include <QSslConfiguration>
#include <QSettings>
#include <QFileInfo>
#include <QProcess>

#include <QPalette>
#include <QStyleFactory>
#include <QFontDatabase>

using namespace Qt::StringLiterals;

#include "launchermainwindow.h"
#include "settings.h"
#include "version.h"
#include "launcheroptions.h"

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
    if(LauncherOptions::isSet("log-debug") == true){
        qInstallMessageHandler(launcherLogFileHandler);
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
            // Convert argv to QStringList
            QStringList argumentList;
            for (int i = 1; i < argc; ++i) { // Start from 1 to skip the program name
                argumentList << QString::fromLocal8Bit(argv[i]);
            }
            // Run new process and pipe return value
            return QProcess::execute(argv[0], argumentList);
        }
    #endif

    // Info: Log some stuff
    qInfo() << "Launcher Directory:" << QCoreApplication::applicationDirPath();
    qInfo() << "Launcher Version:" << LAUNCHER_VERSION;

    // Info: OS
    #ifdef WIN32
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

    // Disable SSL verification
    // TODO: eventually add SSL certs
    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    QSslConfiguration::setDefaultConfiguration(sslConfig);

    // Load the launcher and kfx settings
    // Also try and copy over defaults
    Settings::load();

    // Force dark theme
    setDarkTheme();

    // Create the main window and show it
    LauncherMainWindow mainWindow;
    mainWindow.show();

    // Execute main event loop
    return app.exec();
}
