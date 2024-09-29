#include "settings.h"

#include <QCoreApplication>
#include <QSettings>

#include "settingscfgformat.h"

QSettings *Settings::kfxSettings;
QSettings *Settings::launcherSettings;

QVariant Settings::getKfxSetting(QAnyStringView key)
{
    QVariant value = kfxSettings->value(key);
    QString valueString = value.toString();

    if (valueString == "ON" || valueString == "YES" || valueString == "TRUE") {
        return true;
    }

    if (valueString == "OFF" || valueString == "NO" || valueString == "FALSE") {
        return false;
    }

    return value;
}

void Settings::setKfxSetting(QAnyStringView key, const QVariant &value)
{
    if (value.typeId() == QMetaType::Type::Bool && value == true) {
        kfxSettings->setValue(key, "TRUE");
        return;
    }

    if (value.typeId() == QMetaType::Type::Bool && value == false) {
        kfxSettings->setValue(key, "FALSE");
        return;
    }

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

void Settings::copyNewKfxSettingsFromDefault()
{
    // Get the CFG format used by the original keeperfx.cfg
    QSettings::Format settingsCfgFormat = SettingsCfgFormat::registerFormat();

    // Try and load default KFX settings
    // 'keeperfx.cfg' from the app dir
    QSettings *defaultKfxSettings = new QSettings(QCoreApplication::applicationDirPath()
                                                      + "/keeperfx.cfg",
                                                  settingsCfgFormat);

    // File not loaded
    if (defaultKfxSettings->allKeys().isEmpty()) {
        qDebug() << "Default keeperfx.cfg file not loaded. File not found or it contains no keys";
        return;
    }

    // Loop through all keys in the default 'keeperfx.cfg'
    foreach (const QString &key, defaultKfxSettings->allKeys()) {

        // Get the value
        QVariant value = defaultKfxSettings->value(key);
        QString valueString = value.toString();

        // Fix RESIZE_MOVIES
        if (key == "RESIZE_MOVIES") {
            if (valueString == "ON" || valueString == "YES" || valueString == "TRUE") {
                value = "FIT";
            }
        }

        // Check if user kfx settings misses this key
        if (!kfxSettings->contains(key)) {

            // Copy the setting
            kfxSettings->setValue(key, value);
            qDebug() << "Copied kfx setting from defaults:" << key << "=" << value.toString();
        }
    }
}
