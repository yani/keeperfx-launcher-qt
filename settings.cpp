#include "settings.h"

#include <QCoreApplication>
#include <QSettings>

#include "settingscfgformat.h"

QSettings *Settings::kfxSettings;
QSettings *Settings::launcherSettings;

QVariant Settings::getKfxSetting(QAnyStringView key)
{
    return kfxSettings->value(key);
}

void Settings::setKfxSetting(QAnyStringView key, const QVariant &value)
{
    kfxSettings->setValue(key, value);
}

QVariant Settings::getLauncherSetting(QAnyStringView key)
{
    return launcherSettings->value(key);
}

void Settings::setLauncherSetting(QAnyStringView key, const QVariant &value)
{
    launcherSettings->setValue(key, value);
}

QFile Settings::getKfxConfigFile()
{
    return QFile(kfxSettings->fileName());
}

void Settings::load()
{
    // Get the CFG format used by the original keeperfx.cfg
    QSettings::Format settingsCfgFormat = SettingsCfgFormat::registerFormat();

    // Create the Settings objects
    // This will setup a handle that will create the files if they don't exist and a setting is set
    kfxSettings = new QSettings(
        settingsCfgFormat, QSettings::UserScope, "keeperfx", "keeperfx");
    launcherSettings = new QSettings(
        settingsCfgFormat, QSettings::UserScope, "keeperfx", "launcher");

    // Log the paths
    qDebug() << "KeeperFX Settings File (User):" << kfxSettings->fileName();
    qDebug() << "Launcher Settings File (User):" << launcherSettings->fileName();
}
