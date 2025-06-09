#include "settings.h"

#include <QCoreApplication>
#include <QLocale>
#include <QSettings>

#include "kfxversion.h"
#include "settingscfgformat.h"

QSettings *Settings::kfxSettings;
QSettings *Settings::launcherSettings;

// clang-format off

QMap<QString, QVariant> Settings::defaultLauncherSettingsMap = {

    // Launcher settings
    {"CHECK_FOR_UPDATES_ENABLED", true},
    {"CHECK_FOR_UPDATES_RELEASE", "STABLE"},
    {"AUTO_UPDATE", false},
    {"BACKUP_SAVES", false},
    {"WEBSITE_INTEGRATION_ENABLED", true},
    {"CRASH_REPORTING_ENABLED", true},
    {"CRASH_REPORTING_CONTACT", ""},
    {"OPEN_ON_GAME_SCREEN", false},
    {"GAME_HEAVY_LOG_ENABLED", false},
    {"PLAY_BUTTON_THEME", "dk-orange"},
    {"LAUNCHER_LANGUAGE", QLocale(QLocale::system().uiLanguages().value(0, QLocale::system().name())).name().left(2)}, // Get 2 letter language identifier

    // Stuff to remember
    {"SUPPRESS_ORIGINAL_DK_FOUND_MESSAGEBOX", false},

    // Game executable parameters
    // These also go in the launcher config
    {"GAME_PARAM_NO_SOUND", false},
    {"GAME_PARAM_USE_CD_MUSIC", false},
    {"GAME_PARAM_NO_INTRO", false},
    {"GAME_PARAM_ALEX", false},
    {"GAME_PARAM_FPS", "20"},
    {"GAME_PARAM_HUMAN_PLAYER", "0"},
    {"GAME_PARAM_VID_SMOOTH", false},
    {"GAME_PARAM_PACKET_SAVE_ENABLED", false},
    {"GAME_PARAM_PACKET_SAVE_FILE_NAME", "packetsave.pck"},
};

QMap<QString, QString> Settings::gameSettingsParameterMap = {
    {"GAME_PARAM_NO_SOUND", "-nosound"},
    {"GAME_PARAM_USE_CD_MUSIC", "-cd"},
    {"GAME_PARAM_ALEX", "-alex"},
    {"GAME_PARAM_VID_SMOOTH", "-vidsmooth"},
    // {"GAME_PARAM_FPS", "-fps %d"}, // Hardcoded
    // {"GAME_PARAM_HUMAN_PLAYER", "-human %d"}, // Hardcoded
    // {"GAME_PARAM_PACKET_SAVE_FILE_NAME", "-packetsave %s"}, // Hardcoded
    // {"GAME_PARAM_NO_INTRO", "-nointro"}, // Hardcoded
};

// clang-format on

QMap<QString, QString> Settings::localeToGameLanguageMap = {
    {"en", "ENG"},
    {"it", "ITA"},
    {"fr", "FRE"},
    {"es", "SPA"},
    {"nl", "DUT"},
    {"de", "GER"},
    {"pl", "POL"},
    {"sv", "SWE"},
    {"ja", "JAP"},
    {"ru", "RUS"},
    {"ko", "KOR"},
    {"cs", "CZE"},
    {"la", "LAT"},
    {"zh-Hans", "CHI"}, // Simplified Chinese
    {"zh-Hant", "CHT"}, // Traditional Chinese
};

QVariant Settings::getKfxSetting(QAnyStringView key)
{
    // Get value as a string
    QVariant value = kfxSettings->value(key);
    QString valueString = value.toString();

    // Fix for 'RESIZE_MOVIES' KeeperFX setting
    // This setting is set to "ON" by default which would make this a boolean
    // However, ON should be a string here and is treated different than all the rest
    // Therefor we simply return the value as a string
    if (key.toString() == "RESIZE_MOVIES") {
        return valueString;
    }

    // True strings
    if (valueString == "ON" || valueString == "YES" || valueString == "TRUE") {
        return true;
    }

    // False strings
    if (valueString == "OFF" || valueString == "NO" || valueString == "FALSE") {
        return false;
    }

    // Return the string value directly
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
    if (KfxVersion::hasFunctionality("absolute_config_path") == false) {
        // Use 'keeperfx.cfg' in app folder if kfx version is too old
        // This is bad because eventually we don't want to change any files in the
        // application folder. But older kfx versions do not allow passing a custom config path.
        kfxSettings = new QSettings(QCoreApplication::applicationDirPath() + "/keeperfx.cfg",
                                    settingsCfgFormat);

        qInfo() << "KeeperFX Settings File (App):" << kfxSettings->fileName();

    } else {
        // Put kfx settings in a user folder
        kfxSettings = new QSettings(
            settingsCfgFormat, QSettings::UserScope, "keeperfx", "keeperfx");

        // Copy any settings that are present in the 'keeperfx.cfg' file in the application dir
        copyMissingDefaultSettings();

        qInfo() << "KeeperFX Settings File (User):" << kfxSettings->fileName();
    }

    // Copy missing alpha settings
    // This would be the '_keeperfx.cfg' file
    copyMissingAlphaSettings();

    // Launcher settings
    // Will create a file in a user folder
    launcherSettings = new QSettings(
        settingsCfgFormat, QSettings::UserScope, "keeperfx", "launcher");

    // Log the paths
    qInfo() << "Launcher Settings File (User):" << launcherSettings->fileName();

    // Copy missing launcher settings
    Settings::copyMissingLauncherSettings();
}

void Settings::copyMissingSettings(QSettings *fromSettingsFile, QSettings *toSettingsFile)
{
    // File not loaded
    if (fromSettingsFile->allKeys().isEmpty()) {
        qDebug() << "New settings file not loaded. File not found or it contains no keys";
        return;
    }

    // Loop through all keys in the new settings file default 'keeperfx.cfg'
    foreach (const QString &key, fromSettingsFile->allKeys()) {
        // Get the value
        QVariant value = fromSettingsFile->value(key);
        QString valueString = value.toString();

        // Fix RESIZE_MOVIES
        if (key == "RESIZE_MOVIES") {
            if (valueString == "ON" || valueString == "YES" || valueString == "TRUE") {
                value = "FIT";
            }
        }

        // Check if user kfx settings misses this key
        if (!toSettingsFile->contains(key)) {
            // Copy the setting
            toSettingsFile->setValue(key, value);
            qDebug() << "Copied setting:" << key << "=" << value.toString();
        }
    }
}

void Settings::copyMissingDefaultSettings()
{
    // Try and load default KFX settings
    // 'keeperfx.cfg' from the app dir
    QSettings *defaultKfxSettings = new QSettings(
        QCoreApplication::applicationDirPath() + "/keeperfx.cfg",
        SettingsCfgFormat::registerFormat()
        );

    copyMissingSettings(defaultKfxSettings, kfxSettings);

}

void Settings::copyMissingAlphaSettings()
{
    // Try and load default KFX settings
    // 'keeperfx.cfg' from the app dir
    QSettings *newAlphaSettings = new QSettings(
        QCoreApplication::applicationDirPath() + "/_keeperfx.cfg",
        SettingsCfgFormat::registerFormat()
    );

    copyMissingSettings(newAlphaSettings, kfxSettings);
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

    // Add no intro
    // We hardcode this one because it's different for game versions that have the STARTUP cfg option
    if (KfxVersion::hasFunctionality("startup_config_option") == false) {
        if (Settings::getLauncherSetting("GAME_PARAM_NO_INTRO").toBool() == true) {
            paramList << "-nointro";
        }
    }

    // Add FPS
    QString fps = Settings::getLauncherSetting("GAME_PARAM_FPS").toString();
    if (fps != Settings::defaultLauncherSettingsMap["GAME_PARAM_FPS"]) {
        paramList << "-fps" << fps;
    }

    // Add human player
    QString humanPlayer = Settings::getLauncherSetting("GAME_PARAM_HUMAN_PLAYER").toString();
    if (humanPlayer != Settings::defaultLauncherSettingsMap["GAME_PARAM_HUMAN_PLAYER"]) {
        paramList << "-human" << humanPlayer;
    }

    // Add packetsave
    if (Settings::getLauncherSetting("GAME_PARAM_PACKET_SAVE_ENABLED") == true) {
        QString packetSaveFileName = Settings::getLauncherSetting("GAME_PARAM_PACKET_SAVE_FILE_NAME").toString();
        if (packetSaveFileName.isEmpty() == false) {
            paramList << "-packetsave" << packetSaveFileName;
        }
    }

    return paramList;
}

bool Settings::autoSetGameLanguageToLocaleLanguage()
{
    // Get system locale language as string
    QString localeLanguage = QLocale::system().bcp47Name();

    // Check if we have this language in our map
    if (localeToGameLanguageMap[localeLanguage].isEmpty()) {
        return false;
    }

    // Update game language
    Settings::setKfxSetting("GAME_LANGUAGE", localeToGameLanguageMap[localeLanguage]);

    return true;
}

void Settings::resetKfxSettings()
{
    if (Settings::kfxSettings) {
        Settings::kfxSettings->clear();
        Settings::copyMissingDefaultSettings();
    }
}

void Settings::resetLauncherSettings()
{
    if (Settings::launcherSettings) {
        Settings::launcherSettings->clear();
        Settings::copyMissingLauncherSettings();
    }
}
