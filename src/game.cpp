#include "game.h"
#include <QApplication>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include "settings.h"

Game::Game(QObject *parent)
    : QObject(parent)
    , process(new QProcess(this))
{
    // Setup the process
    process->setWorkingDirectory(QApplication::applicationDirPath());

    // Connect the process finished signal to the onProcessFinished slot
    connect(process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            &Game::onProcessFinished);
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
    case LOAD_MAP:
        return "Load Map";
    case LOAD_CAMPAIGN:
        return "Load Campaign";
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
    qInfo() << "Starting gaming ->" << Game::getStringFromStartType(startType);
    qDebug() << "Data[1]" << data1.toString();
    qDebug() << "Data[2]" << data2.toString();
    qDebug() << "Data[3]" << data3.toString();

    // Get the game parameters
    QStringList params = Settings::getGameSettingsParameters();

    // If version is too old for custom -config path we'll use the default 'keeperfx.cfg' instead
    if (Settings::useOldConfigFilePath() == false) {
        // Add '-config' parameter with our custom keeperfx.cfg
        QFileInfo configFileInfo(Settings::getKfxConfigFile());
        params << "-config" << QDir::toNativeSeparators(configFileInfo.absoluteFilePath());
    } else {
        qWarning() << "Game version too old for custom config path";
    }

    // Log parameters
    if(params.count() > 0){
        qInfo() << "Game parameters:" << params.join(" ");
    } else {
        qInfo() << "No game parameters set";
    }

    // Get the game binary
    // For now it's only the .exe release
    QString keeperfxBin = QApplication::applicationDirPath() + "/keeperfx.exe";

// Start the process
#ifdef Q_OS_WINDOWS
    process->start(keeperfxBin, params);
#else
    params.prepend(keeperfxBin);
    process->start("wine", params);
#endif

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
    emit gameEnded(exitCode, exitStatus);
}

QString Game::getErrorString()
{
    return this->errorString;
}
