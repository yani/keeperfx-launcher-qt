#include <QApplication>
#include <QGuiApplication>
#include <QSslConfiguration>

#include "launchermainwindow.h"
#include "version.h"

int main(int argc, char *argv[])
{
    // Create the App
    QApplication app(argc, argv);

    // Set application details
    // This is used for QSettings which is used for persistant user settings
    QCoreApplication::setOrganizationName("KeeperFX");
    QCoreApplication::setOrganizationDomain("keeperfx.net");
    QCoreApplication::setApplicationName("KeeperFX");

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

    // Disable SSL verification
    // TODO: eventually add SSL certs
    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    QSslConfiguration::setDefaultConfiguration(sslConfig);

    // Create the main window and show it
    LauncherMainWindow mainWindow;
    mainWindow.show();

    // Execute main event loop
    return app.exec();
}
