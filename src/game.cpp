#include "game.h"
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include "crashdialog.h"
#include "kfxversion.h"
#include "settings.h"

Game::Game(QWidget *parent)
    : QObject(parent)
    , process(new QProcess(this))
    , parentWidget(parent)
{
    // Setup the process
    process->setWorkingDirectory(QApplication::applicationDirPath());

    // Connect the process finished signal to the onProcessFinished slot
    connect(process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            &Game::onProcessFinished);
    /*connect(process, &QProcess::readyReadStandardOutput, this, &Game::handleProcessOutput);
    connect(process, &QProcess::readyReadStandardError, this, &Game::handleProcessOutput);*/

}

QString Game::getStringFromStartType(StartType startType)
{
    switch (startType) {
    case NORMAL:
        return "Normal";
    case HVLOG:
        return "Heavy Log";
    case DIRECT_CONNECT:
        return "Direct Connect";
    case MAP:
        return "Map";
    case CAMPAIGN:
        return "Campaign";
    case LOAD_SAVE:
        return "Load Save";
    case LOAD_PACKETSAVE:
        return "Load Packetsave";
    }
    return "Unknown start type";
}

bool Game::start(StartType startType, QVariant data1, QVariant data2, QVariant data3)
{
    // Refresh error
    this->errorString = QString();

    // Log some stuff
    qInfo() << "Setting up game for start";
    qInfo() << "Start type:" << Game::getStringFromStartType(startType);
    qDebug() << "Data[1]" << data1.toString();
    qDebug() << "Data[2]" << data2.toString();
    qDebug() << "Data[3]" << data3.toString();

    // Get the game parameters
    QStringList params = Settings::getGameSettingsParameters();

    // If version is too old for custom -config path we'll use the default 'keeperfx.cfg' instead
    if (KfxVersion::hasFunctionality("absolute_config_path") == true) {
        // Add '-config' parameter with our custom keeperfx.cfg
        QFileInfo configFileInfo(Settings::getKfxConfigFile());
        params << "-config" << QDir::toNativeSeparators(configFileInfo.absoluteFilePath());
    } else {
        qWarning() << "Game version too old for custom config path";
    }

    // Campaign
    if (startType == StartType::CAMPAIGN) {
        params << "-campaign" << data1.toString();
    }

    // Log parameters
    if (params.count() > 0) {
        qInfo() << "Game parameters:" << params.join(" ");
    } else {
        qInfo() << "No game parameters set";
    }

    // Get the game binary
    // For now it's only the .exe release
    QString keeperfxBin;
    if (Settings::getLauncherSetting("GAME_HEAVY_LOG_ENABLED").toBool() == true) {
        keeperfxBin = QApplication::applicationDirPath() + "/keeperfx_hvlog.exe";
    } else {
        keeperfxBin = QApplication::applicationDirPath() + "/keeperfx.exe";
    }

    // Start the process
    #ifdef Q_OS_WINDOWS
        qInfo() << "Starting game (Windows)";
        process->start(keeperfxBin, params);
    #else
        if (qEnvironmentVariableIsSet("FLATPAK_ID")) {
            qInfo() << "Starting game (Linux: Flatpak -> Wine)";
            // Run Wine outside Flatpak
            params.prepend(keeperfxBin);
            params.prepend("wine");
            params.prepend("--host");
            process->start("flatpak-spawn", params);
        } else {
            qInfo() << "Starting game (Linux: Wine)";
            // Normal Wine execution
            params.prepend(keeperfxBin);
            process->start("wine", params);
        }
    #endif

    // Log the full command line
    qDebug() << "Full command:" << process->program() + " " + process->arguments().join(" ");

    // Wait for process to start and check errors
    if (!process->waitForStarted()) {
        this->errorString = process->errorString();
        qDebug() << "Error: Process failed to start:" << process->errorString();
        return false;
    }

    return true;
}

void Game::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    // Check for crash
    if (exitStatus == QProcess::ExitStatus::CrashExit || exitCode != 0) {

        // Open crash dialog if enabled
        if(Settings::getLauncherSetting("CRASH_REPORTING_ENABLED") == true){

            // Create dialog
            CrashDialog crashDialog(this->parentWidget);

            // Check if there is process output and add it to the dialog object
            QString stdErrorString = this->process->readAllStandardError();
            if (stdErrorString.isEmpty() == false) {
                qDebug() << "Process StdError:" << stdErrorString;
                crashDialog.setStdErrorString(stdErrorString);
            }

            // Open the crash dialog
            crashDialog.exec();
        }
    }

    emit gameEnded(exitCode, exitStatus);
}

QString Game::getErrorString()
{
    return this->errorString;
}
