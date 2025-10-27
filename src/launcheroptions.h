#pragma once

#include <QString>
#include <QApplication>
#include <QCommandLineParser>

class LauncherOptions
{

public:
    static QCommandLineParser parser;
    static QStringList activeOptions;

    static bool isSet(const QString option);
    static QString getValue(const QString option);

    static QStringList getArguments();
    static void removeArgumentOption(QString option);

    static void processApp(QApplication &app);

private:
    static QStringList arguments;
};
