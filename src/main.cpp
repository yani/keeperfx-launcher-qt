#include <QApplication>
#include <QGuiApplication>
#include <QSslConfiguration>
#include <QSettings>
#include <QFileInfo>

#include <QPalette>
#include <QStyleFactory>
#include <QFontDatabase>

using namespace Qt::StringLiterals;

#include "launchermainwindow.h"
#include "settings.h"
#include "version.h"
#include "launcheroptions.h"

void setDarkTheme() {
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
    qApp->setPalette(darkPalette);
}

void launcherLogFileHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    // Set logfile to '<application filename>.log'
    static QFile logFile(
        QFileInfo(QCoreApplication::applicationFilePath()).baseName() + ".log");

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

    // Load the launcher and kfx settings
    // Also try and copy over defaults
    Settings::load();
    Settings::copyNewKfxSettingsFromDefault();
    Settings::copyMissingLauncherSettings();

    // Force dark theme
    setDarkTheme();

    // Create the main window and show it
    LauncherMainWindow mainWindow;
    mainWindow.show();

    // Execute main event loop
    return app.exec();
}
