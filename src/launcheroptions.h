#pragma once

#include <QString>
#include <QApplication>
#include <QCommandLineParser>

class LauncherOptions
{

public:

    static QCommandLineParser parser;

    static bool isSet(const QString option);
    static QString getValue(const QString option);

    static void processApp(QApplication &app);
};
