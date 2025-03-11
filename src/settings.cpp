#include "settings.h"

#include <QCoreApplication>
#include <QSettings>

#include "kfxversion.h"
#include "settingscfgformat.h"

QSettings *Settings::kfxSettings;
QSettings *Settings::launcherSettings;

const QString Settings::MinKfxVersionForNewConfigLoc("1.2.0.4399");

QMap<QString, QVariant> Settings::defaultLauncherSettingsMap = {

    // Launcher settings
    {"CHECK_FOR_UPDATES_ENABLED", true},
    {"CHECK_FOR_UPDATES_RELEASE", "STABLE"},
    {"WEBSITE_INTEGRATION_ENABLED", true},
    {"CRASH_REPORTING_ENABLED", true},
    {"CRASH_REPORTING_CONTACT", ""},

    // Game executable parameters
    // These also go in the launcher config
    {"GAME_PARAM_NO_SOUND", false},
    {"GAME_PARAM_USE_CD_MUSIC", false},
    {"GAME_PARAM_NO_INTRO", false},
    {"GAME_PARAM_ALEX", false},
    {"GAME_PARAM_FPS", "20"},
    {"GAME_PARAM_VID_SMOOTH", false},
};

QMap<QString, QString> Settings::gameSettingsParameterMap = {
    {"GAME_PARAM_NO_SOUND", "-nosound"},
    {"GAME_PARAM_USE_CD_MUSIC", "-cd"},
    {"GAME_PARAM_NO_INTRO", "-nointro"},
    {"GAME_PARAM_ALEX", "-alex"},
    {"GAME_PARAM_VID_SMOOTH", "-vidsmooth"},
    // {"GAME_PARAM_FPS", "-fps %d"}, // Hardcoded
};

QVariant Settings::getKfxSetting(QAnyStringView key)
{
    QVariant value = kfxSettings->value(key);
    QString valueString = value.toString();

    // Fix for 'RESIZE_MOVIES' KeeperFX setting
    // This setting is set to "ON" by default which would make this a boolean
    // However, ON should be a string here and is treated different than all the rest
    // Therefor we simply return the value as a string
    if (key.toString() == "RESIZE_MOVIES") {
        return valueString;
    }

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

    // KeeperFX settings
    if (Settings::useOldConfigFilePath()) {
        // Use 'keeperfx.cfg' in app folder if kfx version is too old
        // This is bad because eventually we don't want to change any files in the
        // application folder. But older kfx versions do not allow passing a custom config path.
        kfxSettings = new QSettings(QCoreApplication::applicationDirPath() + "/keeperfx.cfg",
                                    settingsCfgFormat);
    } else {
        // Put kfx settings in a user folder
        kfxSettings = new QSettings(
            settingsCfgFormat, QSettings::UserScope, "keeperfx", "keeperfx");
    }

    // Launcher settings
    // Will create a file in a user folder
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

void Settings::copyMissingLauncherSettings()
{
    // Loop trough default launcher settings
    for (auto it = defaultLauncherSettingsMap.begin(); it != defaultLauncherSettingsMap.end(); ++it) {

        // Check if we are missing this setting
        if (!launcherSettings->contains(it.key())) {

            // Copy the setting
            launcherSettings->setValue(it.key(), it.value());
            qDebug() << "Copied launcher setting from defaults:" << it.key() << "=" << it.value().toString();
        }
    }
}

QStringList Settings::getGameSettingsParameters()
{
    QStringList paramList;

    // Loop trough parameters
    for (auto it = Settings::gameSettingsParameterMap.begin(); it != Settings::gameSettingsParameterMap.end(); ++it) {
        if (Settings::getLauncherSetting(it.key()).toBool() == true) {
            paramList << it.value();
        }
    }

    // Add FPS
    QString fps = Settings::getLauncherSetting("GAME_PARAM_FPS").toString();
    if (fps != Settings::defaultLauncherSettingsMap["GAME_PARAM_FPS"]) {
        paramList << QString("-fps " + fps);
    }

    return paramList;
}

bool Settings::useOldConfigFilePath()
{
    return KfxVersion::isVersionLowerOrEqual(
        KfxVersion::currentVersion.version,
        Settings::MinKfxVersionForNewConfigLoc
    );
}
