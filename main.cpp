#include <QApplication>
#include <QGuiApplication>
#include <QSslConfiguration>
#include <QSettings>

#include "launchermainwindow.h"
#include "settings.h"
#include "version.h"
#include "launcheroptions.h"

int main(int argc, char *argv[])
{
    // Create the App
    QApplication app(argc, argv);
    QApplication::setApplicationName("KeeperFX Launcher");
    QApplication::setApplicationVersion(LAUNCHER_VERSION);

    // DEBUG: Log some stuff
    qDebug() << "Launcher Directory:" << QCoreApplication::applicationDirPath();
    qDebug() << "Launcher Version:" << LAUNCHER_VERSION;

    // DEBUG: OS
    #ifdef WIN32
        qDebug() << "Launcher Build: Windows";
    #else
        qDebug() << "Launcher Build: UNIX";
    #endif

    // DEBUG: Platform
    qDebug() << "Platform:" << QGuiApplication::platformName();

    // Parse launcher options
    LauncherOptions::processApp(app);

    // Disable SSL verification
    // TODO: eventually add SSL certs
    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    QSslConfiguration::setDefaultConfiguration(sslConfig);

    // Load the launcher and kfx settings
    // Also try and copy over defaults
    Settings::load();
    Settings::copyNewKfxSettingsFromDefault();
    Settings::copyMissingLauncherSettings();

    // Create the main window and show it
    LauncherMainWindow mainWindow;
    mainWindow.show();

    // Execute main event loop
    return app.exec();
}
