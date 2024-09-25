#include "settings.h"

#include <QCoreApplication>
#include <QSettings>

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
    kfxSettings = new QSettings(
        QSettings::IniFormat, QSettings::UserScope, "KeeperFX", "KeeperFX");
    launcherSettings = new QSettings(
        QSettings::IniFormat, QSettings::UserScope,"KeeperFX","Launcher");
}
