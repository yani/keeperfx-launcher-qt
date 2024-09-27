#pragma once

#include <QFile>
#include <QString>
#include <QSettings>

class Settings
{
public:

    static QVariant getKfxSetting(QAnyStringView key);
    static void setKfxSetting(QAnyStringView key, const QVariant &value);

    static QVariant getLauncherSetting(QAnyStringView key);
    static void setLauncherSetting(QAnyStringView key, const QVariant &value);

    static QFile getKfxConfigFile();

    static void load();
    static void copyNewKfxSettingsFromDefault();

private:

    static QSettings *kfxSettings;
    static QSettings *launcherSettings;
};
