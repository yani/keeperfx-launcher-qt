#pragma once

#include <QFile>
#include <QMutex>
#include <QtGlobal>

class Logger
{
public:
    static void setupHandler();

private:
    static QFile *logFile;
    static QMutex logMutex;

    static void handler(QtMsgType type,
        const QMessageLogContext &context,
        const QString &msg);

    static QString getMessageTypeString(QtMsgType type);
};
