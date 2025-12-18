#include "logger.h"
#include "launcheroptions.h"

#include <QDateTime>
#include <QDir>
#include <QTextStream>
#include <QCoreApplication>
#include <QFileInfo>
#include <QMutex>

QFile *Logger::logFile = nullptr;
QMutex Logger::logMutex;

void Logger::handler(QtMsgType type,
    const QMessageLogContext &context,
    const QString &msg)
{
    Q_UNUSED(context);

    QMutexLocker locker(&Logger::logMutex);

    const QString typeStr = Logger::getMessageTypeString(type);
    const QString timeStamp =
        QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    // Console output
    {
        FILE *stream =
            (type == QtDebugMsg || type == QtInfoMsg)
                ? stdout
                : stderr;

        QTextStream consoleOut(stream);
        consoleOut << timeStamp << " " << typeStr << ": " << msg << Qt::endl;
        consoleOut.flush();
    }

    // File output
    if (Logger::logFile && Logger::logFile->isOpen()) {
        QTextStream out(Logger::logFile);
        out << timeStamp << " " << typeStr << ": " << msg << Qt::endl;
        out.flush();
    }

    if (type == QtFatalMsg) {
        if (Logger::logFile && Logger::logFile->isOpen()) {
            Logger::logFile->close();
        }
        abort();
    }
}

void Logger::setupHandler()
{
    if (LauncherOptions::isSet("log-debug")) {
        Logger::logFile = new QFile(
            QCoreApplication::applicationDirPath()
            + QDir::separator()
            + QFileInfo(QCoreApplication::applicationFilePath()).baseName()
            + ".log"
        );

        if (!Logger::logFile->open(QIODevice::Truncate | QIODevice::Append | QIODevice::Text)) {
            delete Logger::logFile;
            Logger::logFile = nullptr;
            qWarning() << "Failed to open log output file";
        }
    }

    qInstallMessageHandler(Logger::handler);
}

QString Logger::getMessageTypeString(QtMsgType type)
{
    switch (type) {
        case QtDebugMsg:    return "[DEBUG]";
        case QtInfoMsg:     return "[INFO]";
        case QtWarningMsg:  return "[WARNING]";
        case QtCriticalMsg: return "[CRITICAL]";
        case QtFatalMsg:    return "[FATAL]";
    }
    return "[UNKNOWN]";
}
