#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include "version.h"
#include "kfxversion.h"
#include "popupsignalcombobox.h"
#include "settings.h"

#include <QEvent>
#include <QMouseEvent>
#include <QScreen>
#include <QPushButton>
#include <QMessageBox>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    // Disable resizing and remove maximize button
    setFixedSize(size());
    setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    setWindowFlag(Qt::MSWindowsFixedSizeDialogHint);

    // Hide 'Multiplayer' tab until a future update requires it
    ui->tabWidget->tabBar()->setTabVisible(4, false);

    // Reset setting has changed variable
    settingHasChanged = false;

    // Make sure this widget starts at the first tab
    // When working in Qt's UI editor you can change it to another tab by accident
    ui->tabWidget->setCurrentIndex(0);

    // Set versions in about label
    ui->labelAbout->setText(ui->labelAbout->text()
                                .replace("<kfx_version>", KfxVersion::currentVersion.fullString)
                                .replace("<launcher_version>", LAUNCHER_VERSION));

    // Setup the 'display monitor' dropdown
    setupDisplayMonitorDropdown();

    // Connect the dialog buttons
    connect(ui->buttonBox->button(QDialogButtonBox::Save),
        &QPushButton::clicked,
        this,
        &SettingsDialog::saveSettings);
    connect(ui->buttonBox->button(QDialogButtonBox::Cancel),
        &QPushButton::clicked,
        this,
        &SettingsDialog::cancel);
    connect(ui->buttonBox->button(QDialogButtonBox::RestoreDefaults),
        &QPushButton::clicked,
        this,
        &SettingsDialog::restoreSettings);

    // Map: Languages
    QMap<QString, QString> languageMap = {
                                          {"English", "ENG"},
                                          {"Italiano", "ITA"},
                                          {"Fran\u00E7ais", "FRE"},
                                          {"Espa\u00F1ol", "SPA"},
                                          {"Nederlands", "DUT"},
                                          {"Deutsch", "GER"},
                                          {"Polski", "POL"},
                                          {"Svenska", "SWE"},
                                          {"\u65E5\u672C\u8A9E", "JAP"},
                                          {"\u0420\u0443\u0441\u0441\u043A\u0438\u0439", "RUS"},
                                          {"\uD55C\uAD6D\uC5B4", "KOR"},
                                          {"\u7B80\u4F53\u4E2D\u6587", "CHI"},
                                          {"\u7E41\u9AD4\u4E2D\u6587", "CHT"},
                                          {"\u010Ce\u0161ka", "CZE"},
                                          {"Lat\u012Bna", "LAT"},
                                          };

    // Add languages
    for (auto it = languageMap.begin(); it != languageMap.end(); ++it) {
        ui->comboBoxLanguage->addItem(it.key(), it.value());
    }

    // Map: Languages
    QMap<QString, QString> screenshotMap = {
                                            {"PNG (Portable Network Graphics)", "PNG"},
                                            {"JPG (Joint photographic experts group)", "JPG"},
                                            {"BMP (Windows bitmap)", "BMP"},
                                            {"RAW (HSI 'mhwanh')", "RAW"},
                                            };

    // Add languages
    for (auto it = screenshotMap.begin(); it != screenshotMap.end(); ++it) {
        ui->comboBoxScreenshots->addItem(it.key(), it.value());
    }

    // Map: Resize movies
    QMap<QString, QString> resizeMoviesMap = {
        {tr("Fit (default)"), "FIT"},
        {tr("Off"), "OFF"},
        {tr("Stretch"), "STRETCH"},
        {tr("Crop"), "CROP"},
        {tr("Pixel perfect"), "PIXELPERFECT"},
        {"4BY3", "4BY3"},
        {"4BY3PP", "4BY3PP"},
    };

    // Add languages
    for (auto it = resizeMoviesMap.begin(); it != resizeMoviesMap.end(); ++it) {
        ui->comboBoxResizeMovies->addItem(it.key(), it.value());
    }

    // Map: Resolutions
    QMap<QString, QString> resolutionsMap = {
        {tr("Match desktop"), "MATCH_DESKTOP"},
        {"640 x 400 (8:5)", "640x400"},
        {"640 x 480 (4:3)", "640x480"},
        {"800 x 600 (4:3)", "800x600"},
        {"1024 x 768 (4:3)", "1024x768"},
        {"1280 x 720 (16:9)", "1280x720"},
        {"1280 x 800 (8:5)", "1280x800"},
        {"1280 x 1024 (5:4)", "1280x1024"},
        {"1366 x 768 (16:9)", "1366x768"},
        {"1440 x 900 (8:5)", "1440x900"},
        {"1536 x 864 (16:9)", "1536x864"},
        {"1600 x 900 (16:9)", "1600x900"},
        {"1600 x 1200 (4:3)", "1600x1200"},
        {"1920 x 1080 (16:9)", "1920x1080"},
        {"1920 x 1200 (8:5)", "1920x1200"},
        {"2560 x 1440 (16:9)", "2560x1440"},
        {"2560 x 1600 (16:10)", "2560x1600"},
        {"2880 x 1800 (16:10)", "2880x1800"},
        {"3440 x 1440 (21:9)", "3440x1440"},
        {"3840 x 2160 (16:9)", "3840x2160"},
        {"4096 x 2160 (17:9)", "4096x2160"},
    };

    // Add Resolutions
    for (auto it = resolutionsMap.begin(); it != resolutionsMap.end(); ++it) {
        // Ingame views
        ui->comboBoxResolution1->addItem(it.key(), it.value());
        ui->comboBoxResolution2->addItem(it.key(), it.value());
        ui->comboBoxResolution3->addItem(it.key(), it.value());
        // Front end views
        ui->comboBoxResolution1_2->addItem(it.key(), it.value());
        ui->comboBoxResolution2_2->addItem(it.key(), it.value());
        ui->comboBoxResolution3_2->addItem(it.key(), it.value());
    }

    // Map: Display mode
    QMap<QString, QString> displayModesMap = {
        {tr("Fullscreen"), "x32"},
        {tr("Windowed"), "w32"},
    };

    // Add display modes
    for (auto it = displayModesMap.begin(); it != displayModesMap.end(); ++it) {
        // Ingame views
        ui->comboBoxDisplayMode1->addItem(it.key(), it.value());
        ui->comboBoxDisplayMode2->addItem(it.key(), it.value());
        ui->comboBoxDisplayMode3->addItem(it.key(), it.value());
        // Front end views
        ui->comboBoxDisplayMode1_2->addItem(it.key(), it.value());
        ui->comboBoxDisplayMode2_2->addItem(it.key(), it.value());
        ui->comboBoxDisplayMode3_2->addItem(it.key(), it.value());
    }

    // Map: Atmospheric dropdown
    QMap<QString, QString> atmoMap = {
                                      {tr("Low"), "LOW"},
                                      {tr("Medium"), "MEDIUM"},
                                      {tr("High"), "HIGH"},
                                      };

    // Add atmo settings
    for (auto it = atmoMap.begin(); it != atmoMap.end(); ++it) {
        ui->comboBoxAtmoFrequency->addItem(it.key(), it.value());
        ui->comboBoxAtmoVolume->addItem(it.key(), it.value());
    }

    // Map: Release dropdown
    QMap<QString, QString> releaseMap = {
                                      {tr("Stable (Default)"), "STABLE"},
                                      {tr("Alpha"), "ALPHA"},
                                      };

    // Add atmo settings
    for (auto it = releaseMap.begin(); it != releaseMap.end(); ++it) {
        ui->comboBoxRelease->addItem(it.key(), it.value());
    }

    // Load the settings
    loadSettings();

    // Set input masks (number only textboxes)
    ui->lineEditApiPort->setValidator(new QIntValidator(0, 65535, this));
    ui->lineEditCreatureFlowerSize->setValidator(new QIntValidator(0, 512, this));
    ui->lineEditHandSize->setValidator(new QIntValidator(0, 500, this));
    ui->lineEditLineBoxSize->setValidator(new QIntValidator(0, 500, this));
    ui->lineEditGameturns->setValidator(new QIntValidator(0, 512, this));

    // Set other input masks
    ui->lineEditCommandChar->setValidator(
        // Matches any printable ASCII character (from space to tilde)
        new QRegularExpressionValidator(QRegularExpression("[ -~]"), this));

    // Connect the raw mouse input checkbox
    connect(ui->checkBoxRawMouseInput, &QCheckBox::checkStateChanged, this, [this]() {
        bool isChecked = ui->checkBoxRawMouseInput->isChecked();
        ui->horizontalSliderMouseSens->setEnabled(!isChecked);
        ui->labelMouseSensPercentage->setEnabled(!isChecked);
        ui->labelMouseSens->setEnabled(!isChecked);
        ui->horizontalSliderMouseSens->setValue(isChecked ? 1 : 100);
        ui->labelMouseSensPercentage->setText(isChecked ? "" : QString("%1%").arg(ui->horizontalSliderMouseSens->value()));
    });

    // Connect the mouse sensitivity slider to update the label when moved
    connect(ui->horizontalSliderMouseSens, &QSlider::valueChanged, this, [this](int value) {
        if (!ui->checkBoxRawMouseInput->isChecked()) {
            ui->labelMouseSensPercentage->setText(QString("%1 %").arg(value));
        }
    });

    // Connect the API enabled checkbox
    connect(ui->checkBoxEnableAPI, &QCheckBox::checkStateChanged, this, [this]() {
        bool isChecked = ui->checkBoxEnableAPI->isChecked();
        ui->labelApiPort->setDisabled(!isChecked);
        ui->lineEditApiPort->setDisabled(!isChecked);
    });

    // Connect the atmospheric sounds enabled checkbox
    connect(ui->checkBoxEnableAtmoSounds, &QCheckBox::checkStateChanged, this, [this]() {
        bool isChecked = ui->checkBoxEnableAtmoSounds->isChecked();
        ui->labelAtmoFrequency->setDisabled(!isChecked);
        ui->labelAtmoVolume->setDisabled(!isChecked);
        ui->comboBoxAtmoFrequency->setDisabled(!isChecked);
        ui->comboBoxAtmoVolume->setDisabled(!isChecked);
    });

    // Add handler to remember when a setting has changed
    // This should be executed at the end when the widgets and their contents are final
    addSettingsChangedHandler();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::loadSettings()
{
    // ========================================================================
    // ================================ GAME ==================================
    // ========================================================================

    ui->comboBoxLanguage->setCurrentIndex(
        ui->comboBoxLanguage->findData(Settings::getKfxSetting("LANGUAGE").toString()));
    ui->checkBoxSkipIntro->setChecked(Settings::getLauncherSetting("GAME_PARAM_NO_INTRO") == true);
    ui->checkBoxDisplaySplashScreens->setChecked(Settings::getKfxSetting("DISABLE_SPLASH_SCREENS")
                                                 == false);
    ui->checkBoxCheats->setChecked(Settings::getLauncherSetting("GAME_PARAM_ALEX") == true);
    ui->checkBoxCensorship->setChecked(Settings::getKfxSetting("CENSORSHIP") == true);
    ui->comboBoxScreenshots->setCurrentIndex(
        ui->comboBoxScreenshots->findData(Settings::getKfxSetting("SCREENSHOT").toString()));
    ui->lineEditGameturns->setText(Settings::getLauncherSetting("GAME_PARAM_FPS").toString());
    ui->lineEditCommandChar->setText(Settings::getKfxSetting("COMMAND_CHAR").toString());
    ui->checkBoxDeltaTime->setChecked(Settings::getKfxSetting("DELTA_TIME") == true);
    ui->checkBoxFreezeGameNoFocus->setChecked(Settings::getKfxSetting("FREEZE_GAME_ON_FOCUS_LOST")
                                              == true);

    // ============================================================================
    // ================================ GRAPHICS ==================================
    // ============================================================================

    popupComboBoxMonitorDisplay->setCurrentIndex(popupComboBoxMonitorDisplay->findData(
        Settings::getKfxSetting("DISPLAY_NUMBER").toString()));
    ui->checkBoxSmoothenVideo->setChecked(Settings::getLauncherSetting("GAME_PARAM_VID_SMOOTH")
                                          == true);

    // Resize movies
    // Small fix because "ON" defaults to "FIT", but we just want "FIT" here
    QString resizeMoviesString = Settings::getKfxSetting("RESIZE_MOVIES").toString();
    if (resizeMoviesString == "ON") {
        resizeMoviesString = "FIT";
    }
    ui->comboBoxResizeMovies->setCurrentIndex(
        ui->comboBoxResizeMovies->findData(resizeMoviesString));

    // Loop trough the in-game resolutions
    int resolutionIndex = 0;
    for (QString resolutionString :
         Settings::getKfxSetting("INGAME_RES").toString().trimmed().split(" ")) {
        // Vars
        QString res, mode;

        // Check for DESKTOP and DESKTOP_FULL
        if (resolutionString == "DESKTOP" || resolutionString == "DESKTOP_FULL") {
            res = "MATCH_DESKTOP";
            if (resolutionString == "DESKTOP") {
                mode = "w32";
            } else if (resolutionString == "DESKTOP_FULL") {
                mode = "x32";
            }
        } else {

            // Use regex to split res and mode
            QRegularExpressionMatch match = QRegularExpression("(x32|w32)$").match(resolutionString);
            if (match.hasMatch() == false) {
                qWarning() << "Invalid resolution in 'keeperfx.cfg':" << resolutionString;
                resolutionIndex++;
                continue;
            }

            // Get vars based on matches
            res = resolutionString.left(match.capturedStart(1)); // Part before x32/w32
            mode = match.captured(1);
        }

        // Update the widgets
        switch (resolutionIndex) {
        case 0:
            ui->comboBoxResolution1->setCurrentIndex(ui->comboBoxResolution1->findData(res));
            ui->comboBoxDisplayMode1->setCurrentIndex(ui->comboBoxDisplayMode1->findData(mode));
            break;
        case 1:
            ui->comboBoxResolution2->setCurrentIndex(ui->comboBoxResolution2->findData(res));
            ui->comboBoxDisplayMode2->setCurrentIndex(ui->comboBoxDisplayMode2->findData(mode));
            break;
        case 2:
            ui->comboBoxResolution3->setCurrentIndex(ui->comboBoxResolution3->findData(res));
            ui->comboBoxDisplayMode3->setCurrentIndex(ui->comboBoxDisplayMode3->findData(mode));
            break;
        }

        resolutionIndex++;
    }

    // Loop trough the front end resolutions
    resolutionIndex = 0;
    for (QString resolutionString : Settings::getKfxSetting("FRONTEND_RES").toString().trimmed().split(" ")) {
        // Vars
        QString res, mode;

        // Check for DESKTOP and DESKTOP_FULL
        if (resolutionString == "DESKTOP" || resolutionString == "DESKTOP_FULL") {
            res = "MATCH_DESKTOP";
            if (resolutionString == "DESKTOP") {
                mode = "w32";
            } else if (resolutionString == "DESKTOP_FULL") {
                mode = "x32";
            }
        } else {
            // Use regex to split res and mode
            QRegularExpressionMatch match = QRegularExpression("(x32|w32)$").match(resolutionString);
            if (match.hasMatch() == false) {
                qWarning() << "Invalid resolution in 'keeperfx.cfg':" << resolutionString;
                resolutionIndex++;
                continue;
            }

            // Get vars based on matches
            res = resolutionString.left(match.capturedStart(1)); // Part before x32/w32
            mode = match.captured(1);
        }

        // Update the widgets
        switch (resolutionIndex) {
        case 0:
            ui->comboBoxResolution1_2->setCurrentIndex(ui->comboBoxResolution1->findData(res));
            ui->comboBoxDisplayMode1_2->setCurrentIndex(ui->comboBoxDisplayMode1->findData(mode));
            break;
        case 1:
            ui->comboBoxResolution2_2->setCurrentIndex(ui->comboBoxResolution2->findData(res));
            ui->comboBoxDisplayMode2_2->setCurrentIndex(ui->comboBoxDisplayMode2->findData(mode));
            break;
        case 2:
            ui->comboBoxResolution3_2->setCurrentIndex(ui->comboBoxResolution3->findData(res));
            ui->comboBoxDisplayMode3_2->setCurrentIndex(ui->comboBoxDisplayMode3->findData(mode));
            break;
        }

        resolutionIndex++;
    }

    ui->lineEditCreatureFlowerSize->setText(Settings::getKfxSetting("CREATURE_STATUS_SIZE").toString());
    ui->lineEditLineBoxSize->setText(Settings::getKfxSetting("LINE_BOX_SIZE").toString());
    ui->lineEditHandSize->setText(Settings::getKfxSetting("HAND_SIZE").toString());

    // =========================================================================
    // ================================ SOUND ==================================
    // =========================================================================

    ui->checkBoxEnableSound->setChecked(Settings::getLauncherSetting("GAME_PARAM_NO_SOUND") == false);
    ui->checkBoxUseCDMusic->setChecked(Settings::getLauncherSetting("GAME_PARAM_USE_CD_MUSIC") == true);
    ui->checkBoxPauseMusicWhenPaused->setChecked(
        Settings::getKfxSetting("PAUSE_MUSIC_WHEN_GAME_PAUSED") == true);
    ui->checkBoxMuteAudioWhenNotFocused->setChecked(
        Settings::getKfxSetting("MUTE_AUDIO_ON_FOCUS_LOST") == true);

    // Atmospheric sounds
    ui->checkBoxEnableAtmoSounds->setChecked(
        Settings::getKfxSetting("ATMOSPHERIC_SOUNDS") == true);
    ui->comboBoxAtmoFrequency->setCurrentIndex(
        ui->comboBoxAtmoFrequency->findData(Settings::getKfxSetting("ATMOS_FREQUENCY").toString()));
    ui->comboBoxAtmoVolume->setCurrentIndex(
        ui->comboBoxAtmoVolume->findData(Settings::getKfxSetting("ATMOS_VOLUME").toString()));

    // Atmospheric checkbox disable / enable extra info
    bool isAtmoSoundsEnabled = Settings::getKfxSetting("ATMOSPHERIC_SOUNDS") == true;
    ui->labelAtmoFrequency->setDisabled(!isAtmoSoundsEnabled);
    ui->labelAtmoVolume->setDisabled(!isAtmoSoundsEnabled);
    ui->comboBoxAtmoFrequency->setDisabled(!isAtmoSoundsEnabled);
    ui->comboBoxAtmoVolume->setDisabled(!isAtmoSoundsEnabled);

    // =========================================================================
    // ================================ INPUT ==================================
    // =========================================================================

    int mouseSens = Settings::getKfxSetting("POINTER_SENSITIVITY").toInt();
    if (mouseSens == 0) {
        ui->checkBoxRawMouseInput->setChecked(true);
        ui->horizontalSliderMouseSens->setDisabled(true);
        ui->labelMouseSensPercentage->setDisabled(true);
        ui->labelMouseSens->setDisabled(true);
        ui->horizontalSliderMouseSens->setValue(1);
        ui->labelMouseSensPercentage->setText("");
    } else {
        ui->checkBoxRawMouseInput->setChecked(false);
        ui->horizontalSliderMouseSens->setDisabled(false);
        ui->labelMouseSensPercentage->setDisabled(false);
        ui->labelMouseSens->setDisabled(false);
        ui->horizontalSliderMouseSens->setValue(mouseSens);
        ui->labelMouseSensPercentage->setText(QString::number(mouseSens) + "%");
    }

    ui->checkBoxAltInput->setChecked(Settings::getLauncherSetting("GAME_PARAM_ALT_INPUT") == true);
    ui->checkBoxUnlockCursorWhenPaused->setChecked(Settings::getKfxSetting("UNLOCK_CURSOR_WHEN_GAME_PAUSED") == true);
    ui->checkBoxLockCursorPossession->setChecked(Settings::getKfxSetting("LOCK_CURSOR_IN_POSSESSION") == true);
    ui->checkBoxScreenEdgePanning->setChecked(Settings::getKfxSetting("CURSOR_EDGE_CAMERA_PANNING") == true);

    // ===============================================================================
    // ================================ MULTIPLAYER ==================================
    // ===============================================================================

    ui->lineEditMasterServer->setText(Settings::getKfxSetting("MASTERSERVER_HOST").toString());

    // =======================================================================
    // ================================ API ==================================
    // =======================================================================

    bool isApiEnabled = Settings::getKfxSetting("API_ENABLED") == true;
    ui->checkBoxEnableAPI->setChecked(isApiEnabled);
    if (isApiEnabled) {
        ui->labelApiPort->setDisabled(false);
        ui->lineEditApiPort->setDisabled(false);
    } else {
        ui->labelApiPort->setDisabled(true);
        ui->lineEditApiPort->setDisabled(true);
    }

    ui->lineEditApiPort->setText(Settings::getKfxSetting("API_PORT").toString());

    // ============================================================================
    // ================================ LAUNCHER ==================================
    // ============================================================================

    ui->checkBoxCheckForUpdates->setChecked(Settings::getLauncherSetting("CHECK_FOR_UPDATES_ENABLED") == true);
    ui->comboBoxRelease->setCurrentIndex(ui->comboBoxRelease->findData(Settings::getLauncherSetting("CHECK_FOR_UPDATES_RELEASE").toString()));
    ui->checkBoxWebsiteIntegration->setChecked(Settings::getLauncherSetting("WEBSITE_INTEGRATION_ENABLED") == true);
    ui->checkBoxCrashReporting->setChecked(Settings::getLauncherSetting("CRASH_REPORTING_ENABLED") == true);
    ui->checkBoxOpenOnGameScreen->setChecked(Settings::getLauncherSetting("OPEN_ON_GAME_SCREEN") == true);
}

void SettingsDialog::saveSettings()
{

    // ========================================================================
    // ================================ GAME ==================================
    // ========================================================================

    Settings::setKfxSetting("LANGUAGE", ui->comboBoxLanguage->currentData().toString());
    Settings::setLauncherSetting("GAME_PARAM_NO_INTRO", ui->checkBoxSkipIntro->isChecked());
    Settings::setKfxSetting("DISABLE_SPLASH_SCREENS",
                            ui->checkBoxDisplaySplashScreens->isChecked() == false);
    Settings::setLauncherSetting("GAME_PARAM_ALEX", ui->checkBoxCheats->isChecked());
    Settings::setKfxSetting("CENSORSHIP", ui->checkBoxCensorship->isChecked());
    Settings::setKfxSetting("SCREENSHOT", ui->comboBoxScreenshots->currentData().toString());
    Settings::setLauncherSetting("GAME_PARAM_FPS", ui->lineEditGameturns->text());
    Settings::setKfxSetting("COMMAND_CHAR", ui->lineEditCommandChar->text());
    Settings::setKfxSetting("DELTA_TIME", ui->checkBoxDeltaTime->isChecked());
    Settings::setKfxSetting("FREEZE_GAME_ON_FOCUS_LOST", ui->checkBoxFreezeGameNoFocus->isChecked());


    // ============================================================================
    // ================================ GRAPHICS ==================================
    // ============================================================================

    Settings::setLauncherSetting("GAME_PARAM_VID_SMOOTH", ui->checkBoxSmoothenVideo->isChecked());
    Settings::setKfxSetting("DISPLAY_NUMBER", popupComboBoxMonitorDisplay->currentData().toString());
    Settings::setKfxSetting("RESIZE_MOVIES", ui->comboBoxResizeMovies->currentData().toString());

    // Save the in-game resolutions
    QString resolutionString = "";
    for (int i = 0; i < 3; i++) {
        // Get vars
        QString res, mode;
        switch (i) {
        case 0:
            res = ui->comboBoxResolution1->currentData().toString();
            mode = ui->comboBoxDisplayMode1->currentData().toString();
            break;
        case 1:
            res = ui->comboBoxResolution2->currentData().toString();
            mode = ui->comboBoxDisplayMode2->currentData().toString();
            break;
        case 2:
            res = ui->comboBoxResolution3->currentData().toString();
            mode = ui->comboBoxDisplayMode3->currentData().toString();
            break;
        }

        // Check if user wants to match their desktop size
        if (res == "MATCH_DESKTOP") {
            resolutionString += "DESKTOP";

            // Check for fullscreen
            if (mode == "x32") {
                resolutionString += "_FULL";
            }
        } else {
            // Add absolute resoltion size
            resolutionString += res;
            resolutionString += mode;
        }

        // Add spaces in between
        if (i != 2) {
            resolutionString += " ";
        }
    }
    Settings::setKfxSetting("INGAME_RES", resolutionString);

    // Save the front end resolutions
    resolutionString = "";
    for (int i = 0; i < 3; i++) {
        // Get vars
        QString res, mode;
        switch (i) {
        case 0:
            res = ui->comboBoxResolution1_2->currentData().toString();
            mode = ui->comboBoxDisplayMode1_2->currentData().toString();
            break;
        case 1:
            res = ui->comboBoxResolution2_2->currentData().toString();
            mode = ui->comboBoxDisplayMode2_2->currentData().toString();
            break;
        case 2:
            res = ui->comboBoxResolution3_2->currentData().toString();
            mode = ui->comboBoxDisplayMode3_2->currentData().toString();
            break;
        }

        // Check if user wants to match their desktop size
        if (res == "MATCH_DESKTOP") {
            resolutionString += "DESKTOP";

            // Check for fullscreen
            if (mode == "x32") {
                resolutionString += "_FULL";
            }
        } else {
            // Add absolute resoltion size
            resolutionString += res;
            resolutionString += mode;
        }

        // Add spaces in between
        if (i != 2) {
            resolutionString += " ";
        }
    }
    Settings::setKfxSetting("FRONTEND_RES", resolutionString);

    Settings::setKfxSetting("CREATURE_STATUS_SIZE", ui->lineEditCreatureFlowerSize->text());
    Settings::setKfxSetting("LINE_BOX_SIZE", ui->lineEditLineBoxSize->text());
    Settings::setKfxSetting("HAND_SIZE", ui->lineEditHandSize->text());

    // =========================================================================
    // ================================ SOUND ==================================
    // =========================================================================

    Settings::setLauncherSetting("GAME_PARAM_NO_SOUND", ui->checkBoxEnableSound->isChecked() == false);
    Settings::setLauncherSetting("GAME_PARAM_USE_CD_MUSIC", ui->checkBoxUseCDMusic->isChecked());
    Settings::setKfxSetting("PAUSE_MUSIC_WHEN_GAME_PAUSED", ui->checkBoxPauseMusicWhenPaused->isChecked());
    Settings::setKfxSetting("MUTE_AUDIO_ON_FOCUS_LOST", ui->checkBoxMuteAudioWhenNotFocused->isChecked());
    Settings::setKfxSetting("ATMOSPHERIC_SOUNDS", ui->checkBoxEnableAtmoSounds->isChecked());
    Settings::setKfxSetting("ATMOS_FREQUENCY", ui->comboBoxAtmoFrequency->currentData().toString());
    Settings::setKfxSetting("ATMOS_VOLUME", ui->comboBoxAtmoVolume->currentData().toString());

    // =========================================================================
    // ================================ INPUT ==================================
    // =========================================================================

    if(ui->checkBoxRawMouseInput->isChecked()){
        Settings::setKfxSetting("POINTER_SENSITIVITY", 0);
    } else {
        Settings::setKfxSetting("POINTER_SENSITIVITY", ui->horizontalSliderMouseSens->value());
    }

    Settings::setLauncherSetting("GAME_PARAM_ALT_INPUT", ui->checkBoxAltInput->isChecked() == true);
    Settings::setKfxSetting("UNLOCK_CURSOR_WHEN_GAME_PAUSED", ui->checkBoxUnlockCursorWhenPaused->isChecked() == true);
    Settings::setKfxSetting("LOCK_CURSOR_IN_POSSESSION", ui->checkBoxLockCursorPossession->isChecked() == true);
    Settings::setKfxSetting("CURSOR_EDGE_CAMERA_PANNING", ui->checkBoxScreenEdgePanning->isChecked() == true);

    // ===============================================================================
    // ================================ MULTIPLAYER ==================================
    // ===============================================================================

    Settings::setKfxSetting("MASTERSERVER_HOST", ui->lineEditMasterServer->text());

    // =======================================================================
    // ================================ API ==================================
    // =======================================================================

    Settings::setKfxSetting("API_ENABLED", ui->checkBoxEnableAPI->isChecked() == true);
    Settings::setKfxSetting("API_PORT", ui->lineEditApiPort->text());

    // ============================================================================
    // ================================ LAUNCHER ==================================
    // ============================================================================

    Settings::setLauncherSetting("CHECK_FOR_UPDATES_ENABLED", ui->checkBoxCheckForUpdates->isChecked() == true);
    Settings::setLauncherSetting("CHECK_FOR_UPDATES_RELEASE", ui->comboBoxRelease->currentData().toString());
    Settings::setLauncherSetting("WEBSITE_INTEGRATION_ENABLED", ui->checkBoxWebsiteIntegration->isChecked() == true);
    Settings::setLauncherSetting("CRASH_REPORTING_ENABLED", ui->checkBoxCrashReporting->isChecked() == true);
    Settings::setLauncherSetting("OPEN_ON_GAME_SCREEN", ui->checkBoxOpenOnGameScreen->isChecked() == true);

    // Close the settings screen
    this->close();
}

void SettingsDialog::restoreSettings()
{
    // restore
}

void SettingsDialog::addSettingsChangedHandler()
{
    // Find all QCheckBox
    QList<QCheckBox *> checkBoxes = ui->tabWidget->findChildren<QCheckBox *>();
    for (QCheckBox *checkBox : checkBoxes) {
        connect(checkBox, &QCheckBox::checkStateChanged, this, [this]() {
            this->settingHasChanged = true;
        });
    }

    // Find all QComboBox
    QList<QComboBox *> comboBoxes = ui->tabWidget->findChildren<QComboBox *>();
    for (QComboBox *comboBox : comboBoxes) {
        connect(comboBox, &QComboBox::currentIndexChanged, this, [this]() {
            this->settingHasChanged = true;
        });
    }

    // Find all QLineEdit
    QList<QLineEdit *> lineEdits = ui->tabWidget->findChildren<QLineEdit *>();
    for (QLineEdit *lineEdit : lineEdits) {
        connect(lineEdit, &QLineEdit::textChanged, this, [this]() {
            this->settingHasChanged = true;
        });
    }

    // Find all PopupSignalComboBox
    QList<PopupSignalComboBox *> popupComboBoxes = ui->tabWidget->findChildren<PopupSignalComboBox *>();
    for (PopupSignalComboBox *popupComboBox : popupComboBoxes) {
        connect(popupComboBox, &PopupSignalComboBox::currentIndexChanged, this, [this]() {
            this->settingHasChanged = true;
        });
    }
}

void SettingsDialog::cancel()
{
    // Check if any settings have been changed
    if (settingHasChanged == true) {

        // Ask if the user is sure
        int result = QMessageBox::question(this, tr("KeeperFX Settings"),
            tr("One or more settings have been changed.") + " " +
            tr("Are you sure you want to return without saving?")
        );

        // Cancel close if user is not sure
        if (result != QMessageBox::Yes) {
            return;
        }
    }

    // Close the settings screen
    this->close();
}

void SettingsDialog::setupDisplayMonitorDropdown()
{
    // Get some details of the placeholder combo box and delete it
    QComboBox *oldComboBox = ui->comboBoxDisplayMonitor;
    QRect geometry = oldComboBox->geometry();
    QWidget *parentWidget = oldComboBox->parentWidget();
    delete ui->comboBoxDisplayMonitor;

    // Create the combobox that handles the popup signal
    // This signal is used to show and hide monitor display numers
    popupComboBoxMonitorDisplay = new PopupSignalComboBox(parentWidget);
    popupComboBoxMonitorDisplay->setGeometry(geometry);

    // Get the list of available screens
    QList<QScreen *> screens = QGuiApplication::screens();

    // Loop through each screen and print index and name
    for (int i = 0; i < screens.size(); ++i) {
        QScreen *screen = screens.at(i);

        int index = i;
        int number = index + 1;
        int roundedRefreshRate = qRound(screen->refreshRate());

        QString name = "#" + QString::number(number) + " " +
                       ((screen->model().isEmpty() == false) ? screen->model(): "-") + " "
                       + QString::number(roundedRefreshRate) + "hz "
                       + QString::number(screen->geometry().width()) + "x"
                       + QString::number(screen->geometry().height());

        popupComboBoxMonitorDisplay->addItem(name, QString::number(number));
    }

    // Connect slots to show and hide monitor overlays
    QObject::connect(popupComboBoxMonitorDisplay, &PopupSignalComboBox::popupOpened, this, &SettingsDialog::showMonitorNumberOverlays);
    QObject::connect(popupComboBoxMonitorDisplay, &PopupSignalComboBox::popupClosed, this, &SettingsDialog::hideMonitorNumberOverlays);
}

void SettingsDialog::showMonitorNumberOverlays()
{
    qDebug() << "Show monitor number overlays";

    // We can't show the overlays on wayland
    // They will stack like windows and we can't change their position
    // TODO: monitor when this is possible on wayland and allow it again
    if (QGuiApplication::platformName() == "wayland") {
        qDebug() << "Monitor overlays not shown because running under 'wayland'";
        return;
    }

    // Get the list of all screens
    QList<QScreen *> screens = QGuiApplication::screens();

    // Create a label on each screen at the top-right corner
    for (int i = 0; i < screens.size(); ++i) {

        QScreen *screen = screens[i];

        // Create the overlay widget
        QWidget *overlay = new QWidget;
        overlay->setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint
                                | Qt::BypassWindowManagerHint | Qt::X11BypassWindowManagerHint);
        overlay->setScreen(screen);

        // Set background color to black with
        QPalette palette = overlay->palette();
        palette.setColor(QPalette::Window, QColor(0, 0, 0));
        overlay->setAutoFillBackground(true);
        overlay->setPalette(palette);

        // Set size and position to top-right of the respective screen
        QRect screenGeometry = screen->geometry();
        overlay->setGeometry(screenGeometry.right() - 120, screenGeometry.top() + 20, 100, 100);

        // Create label showing the screen number
        QLabel *label = new QLabel(QString::number(i + 1), overlay);
        label->setStyleSheet("color: white; font-size: 40px;"); // Set text color and size
        label->setAlignment(Qt::AlignCenter);

        QVBoxLayout *layout = new QVBoxLayout(overlay);
        layout->addWidget(label);
        overlay->setLayout(layout);

        overlay->show();
        monitorNumberOverlays.append(overlay); // Keep track of the overlays to remove later

        overlay->setScreen(screen);
    }
}

void SettingsDialog::hideMonitorNumberOverlays()
{
    qDebug() << "Hide monitor number overlays";

    // Remove all the overlays
    for (QWidget *overlay : monitorNumberOverlays) {
        overlay->hide();
        delete overlay;
    }
    monitorNumberOverlays.clear();
}
