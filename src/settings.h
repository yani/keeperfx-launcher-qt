#pragma once

#include <QFile>
#include <QString>
#include <QSettings>

class Settings
{
public:
    static const QString MinKfxVersionForNewConfigLoc;

    static QVariant getKfxSetting(QAnyStringView key);
    static void setKfxSetting(QAnyStringView key, const QVariant &value);

    static QVariant getLauncherSetting(QAnyStringView key);
    static void setLauncherSetting(QAnyStringView key, const QVariant &value);

    static QFile getKfxConfigFile();

    static void load();
    static void copyNewKfxSettingsFromDefault();
    static void copyMissingLauncherSettings();

    static QStringList getGameSettingsParameters();

    static bool useOldConfigFilePath();

private:
    static QMap<QString, QVariant> defaultLauncherSettingsMap;
    static QMap<QString, QString> gameSettingsParameterMap;

    static QSettings *kfxSettings;
    static QSettings *launcherSettings;
};
