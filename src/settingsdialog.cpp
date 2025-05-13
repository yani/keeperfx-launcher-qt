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
    /*connect(ui->buttonBox->button(QDialogButtonBox::RestoreDefaults),
        &QPushButton::clicked,
        this,
        &SettingsDialog::restoreSettings);*/

    // Disable Save button when nothing is changed
    ui->buttonBox->button(QDialogButtonBox::Save)->setDisabled(true);

    // Game languages dropdown
    ui->comboBoxLanguage->addItem("English", "ENG");                                    // English
    ui->comboBoxLanguage->addItem("Italiano", "ITA");                                   // Italian
    ui->comboBoxLanguage->addItem("Fran\u00E7ais", "FRE");                              // French
    ui->comboBoxLanguage->addItem("Espa\u00F1ol", "SPA");                               // Spanish
    ui->comboBoxLanguage->addItem("Nederlands", "DUT");                                 // Dutch
    ui->comboBoxLanguage->addItem("Deutsch", "GER");                                    // German
    ui->comboBoxLanguage->addItem("Polski", "POL");                                     // Polish
    ui->comboBoxLanguage->addItem("Svenska", "SWE");                                    // Swedish
    ui->comboBoxLanguage->addItem("\u65E5\u672C\u8A9E", "JAP");                         // Japanese
    ui->comboBoxLanguage->addItem("\u0420\u0443\u0441\u0441\u043A\u0438\u0439", "RUS"); // Russian
    ui->comboBoxLanguage->addItem("\uD55C\uAD6D\uC5B4", "KOR");                         // Korean
    ui->comboBoxLanguage->addItem("\u7B80\u4F53\u4E2D\u6587", "CHI");                   // Chinese (Simplified)
    ui->comboBoxLanguage->addItem("\u7E41\u9AD4\u4E2D\u6587", "CHT");                   // Chinese (Traditional)
    ui->comboBoxLanguage->addItem("\u010Ce\u0161ka", "CZE");                            // Czech
    ui->comboBoxLanguage->addItem("Lat\u012Bna", "LAT");                                // Latin

    // Add Ukrainian Game Language
    // We do this after the list of languages because this one was added later
    if (KfxVersion::hasFunctionality("ukrainian_game_language") == true) {
        ui->comboBoxLanguage->addItem("\u0423\u043A\u0440\u0430\u0457\u043D\u0441\u044C\u043A\u0430", "UKR"); // Ukrainian
    }

    // Player colors
    ui->comboBoxHumanPlayer->addItem(tr("0: Red (Default)", "Player Color Dropdown"), "0");
    ui->comboBoxHumanPlayer->addItem(tr("1: Blue", "Player Color Dropdown"), "1");
    ui->comboBoxHumanPlayer->addItem(tr("2: Green", "Player Color Dropdown"), "2");
    ui->comboBoxHumanPlayer->addItem(tr("3: Yellow", "Player Color Dropdown"), "3");
    ui->comboBoxHumanPlayer->addItem(tr("4: White / Heroes", "Player Color Dropdown"), "4");
    // 5 = neutral but that would just break

    // Add new player colors
    if (KfxVersion::hasFunctionality("player_colors_purple_orange_black") == true) {
        ui->comboBoxHumanPlayer->addItem(tr("6: Purple", "Player Color Dropdown"), "6");
        ui->comboBoxHumanPlayer->addItem(tr("7: Black", "Player Color Dropdown"), "7");
        ui->comboBoxHumanPlayer->addItem(tr("8: Orange", "Player Color Dropdown"), "8");
    }

    // Launcher language dropdown
    ui->comboBoxLauncherLanguage->addItem("English", "en");    // English
    ui->comboBoxLauncherLanguage->addItem("Nederlands", "nl"); // Dutch
    //ui->comboBoxLauncherLanguage->addItem("Italiano", "it"); // Italian
    //ui->comboBoxLauncherLanguage->addItem("Fran\u00E7ais", "fr"); // French
    //ui->comboBoxLauncherLanguage->addItem("Espa\u00F1ol", "es"); // Spanish
    //ui->comboBoxLauncherLanguage->addItem("Deutsch", "de"); // German
    //ui->comboBoxLauncherLanguage->addItem("Polski", "pl"); // Polish
    //ui->comboBoxLauncherLanguage->addItem("Svenska", "sv"); // Swedish
    //ui->comboBoxLauncherLanguage->addItem("\u65E5\u672C\u8A9E", "ja"); // Japanese
    //ui->comboBoxLauncherLanguage->addItem("\u0420\u0443\u0441\u0441\u043A\u0438\u0439", "ru"); // Russian
    //ui->comboBoxLauncherLanguage->addItem("\uD55C\uAD6D\uC5B4", "ko"); // Korean
    //ui->comboBoxLauncherLanguage->addItem("\u7B80\u4F53\u4E2D\u6587", "zh-Hans"); // Chinese (Simplified)
    //ui->comboBoxLauncherLanguage->addItem("\u7E41\u9AD4\u4E2D\u6587", "zh-Hant"); // Chinese (Traditional)
    ui->comboBoxLauncherLanguage->addItem("\u010Ce\u0161ka", "cs"); // Czech
    //ui->comboBoxLauncherLanguage->addItem("Lat\u012Bna", "la"); // Latin
    //ui->comboBoxLauncherLanguage->addItem("\u0423\u043A\u0440\u0430\u0454\u043D\u044C\u0441\u043A\u0430", "uk"); // Ukrainian

    // Launcher play button theme dropdown
    ui->comboBoxPlayButtonTheme->addItem(tr("Qt Fusion Dark", "Play Button Theme Dropdown"), "qt-fusion-dark");
    ui->comboBoxPlayButtonTheme->addItem(tr("DK Orange (default)", "Play Button Theme Dropdown"), "dk-orange");

    // Screenshot type dropdown
    ui->comboBoxScreenshots->addItem("PNG (Portable Network Graphics)", "PNG");
    ui->comboBoxScreenshots->addItem("JPG (Joint photographic experts group)", "JPG");
    ui->comboBoxScreenshots->addItem("BMP (Windows bitmap)", "BMP");
    ui->comboBoxScreenshots->addItem("RAW (HSI 'mhwanh')", "RAW");

    // Resize movies dropdown
    ui->comboBoxResizeMovies->addItem(tr("Fit (default)", "Resize Movies Dropdown"), "FIT");
    ui->comboBoxResizeMovies->addItem(tr("Off", "Resize Movies Dropdown"), "OFF");
    ui->comboBoxResizeMovies->addItem(tr("Stretch", "Resize Movies Dropdown"), "STRETCH");
    ui->comboBoxResizeMovies->addItem(tr("Crop", "Resize Movies Dropdown"), "CROP");
    ui->comboBoxResizeMovies->addItem(tr("Pixel perfect", "Resize Movies Dropdown"), "PIXELPERFECT");
    ui->comboBoxResizeMovies->addItem("4BY3", "4BY3");
    ui->comboBoxResizeMovies->addItem("4BY3PP", "4BY3PP");

    // Map: Resolutions
    QMap<QString, QString> resolutionsMap = {
        {tr("Match desktop", "Resolution Dropdown"), "MATCH_DESKTOP"},
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
        {tr("Fullscreen", "Display Mode Dropdown"), "x32"},
        {tr("Windowed", "Display Mode Dropdown"), "w32"},
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

    // Atmosphere Frequency dropdown
    ui->comboBoxAtmoFrequency->addItem(tr("Low", "Atmosphere Frequency Dropdown"), "LOW");
    ui->comboBoxAtmoFrequency->addItem(tr("Medium", "Atmosphere Frequency Dropdown"), "MEDIUM");
    ui->comboBoxAtmoFrequency->addItem(tr("High", "Atmosphere Frequency Dropdown"), "HIGH");

    // Atmosphere Volume dropdown
    ui->comboBoxAtmoVolume->addItem(tr("Low", "Atmosphere Volume Dropdown"), "LOW");
    ui->comboBoxAtmoVolume->addItem(tr("Medium", "Atmosphere Volume Dropdown"), "MEDIUM");
    ui->comboBoxAtmoVolume->addItem(tr("High", "Atmosphere Volume Dropdown"), "HIGH");

    // Release dropdown
    ui->comboBoxRelease->addItem(tr("Stable (Default)", "Game Release Build"), "STABLE");
    ui->comboBoxRelease->addItem(tr("Alpha", "Game Release Build"), "ALPHA");

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

    // Connect the game update checkbox
    connect(ui->checkBoxCheckForUpdates, &QCheckBox::checkStateChanged, this, [this]() {
        bool isChecked = ui->checkBoxCheckForUpdates->isChecked();
        ui->labelRelease->setDisabled(!isChecked);
        ui->comboBoxRelease->setDisabled(!isChecked);
        ui->checkBoxAutoUpdate->setDisabled(!isChecked);
        ui->checkBoxBackupSaves->setDisabled(!isChecked);
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

    // Map: Libraries for about page
    QMap<QString, QString> aboutLibrariesMap = {
        {"Qt6", "https://www.qt.io/product/qt6"},
        {"libLIEF", "https://github.com/lief-project/LIEF"},
        {"bit7z", "https://github.com/rikyoz/bit7z"},
        {"7z", "https://www.7-zip.org/"},
        {"zlib", "https://www.zlib.net/"},
    };

    // Create libraries string
    QStringList libraries;
    for (auto it = aboutLibrariesMap.begin(); it != aboutLibrariesMap.end(); ++it) {
        libraries << "<a href=\"" + it.value() + "\">" + it.key() + "</a>";
    }

    // Load about page string
    QString aboutString;
    aboutString += tr("KeeperFX v%1", "About Label").arg(KfxVersion::currentVersion.fullString);
    aboutString += "<br/>";
    aboutString += "<a href=\"https://keeperfx.net\">https://keeperfx.net</a>";
    aboutString += "<br/>";
    aboutString += "<br/>";
    aboutString += tr("KeeperFX Launcher v%1", "About Label").arg(LAUNCHER_VERSION);
    aboutString += "<br/>";
    aboutString += tr("Launcher Libraries:", "About Label");
    aboutString += "<br/>";
    aboutString += libraries.join(", ");
    aboutString += "<br/>";
    aboutString += "<br/>";
    aboutString += tr("Join us Discord: %1", "About Label").arg(QString("<a href=\"%1\">%2</a>").arg("https://discord.gg/WxgE8WZBku").arg("Keeper Klan Discord"));

    // Load about page
    ui->labelAbout->setTextFormat(Qt::RichText);
    ui->labelAbout->setTextInteractionFlags(Qt::TextBrowserInteraction);
    ui->labelAbout->setOpenExternalLinks(true);
    ui->labelAbout->setText(aboutString);

    // Load contributors
    QFile contributorsFile(":/res/contributors.txt");
    if (contributorsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QStringList contributors = QString::fromUtf8(contributorsFile.readAll()).split('\n', Qt::SkipEmptyParts);

        // Convert string of contributors in GitHub links
        for (QString &c : contributors) {
            c = QString("<a href=\"https://github.com/%1\">%1</a>").arg(c.trimmed());
        }

        // Setup label properties
        ui->labelContributors->setTextFormat(Qt::RichText);
        ui->labelContributors->setTextInteractionFlags(Qt::TextBrowserInteraction);
        ui->labelContributors->setOpenExternalLinks(true);

        // Set text
        ui->labelContributors->setText(tr("KeeperFX Contributors: %1", "Label Contributor List").arg(contributors.join(", ")));
    } else {
        ui->labelContributors->setText(""); // Hide
    }
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
    ui->comboBoxHumanPlayer->setCurrentIndex(ui->comboBoxHumanPlayer->findData(Settings::getLauncherSetting("GAME_PARAM_HUMAN_PLAYER").toString()));

    // ============================================================================
    // ================================ GRAPHICS ==================================
    // ============================================================================

    popupComboBoxMonitorDisplay->setCurrentIndex(popupComboBoxMonitorDisplay->findData(Settings::getKfxSetting("DISPLAY_NUMBER").toString()));
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
    ui->lineEditApiPort->setText(Settings::getKfxSetting("API_PORT").toString());

    ui->labelApiPort->setDisabled(!isApiEnabled);
    ui->lineEditApiPort->setDisabled(!isApiEnabled);

    // ============================================================================
    // ================================ LAUNCHER ==================================
    // ============================================================================

    ui->comboBoxLauncherLanguage->setCurrentIndex(ui->comboBoxLauncherLanguage->findData(Settings::getLauncherSetting("LAUNCHER_LANGUAGE").toString()));
    ui->checkBoxWebsiteIntegration->setChecked(Settings::getLauncherSetting("WEBSITE_INTEGRATION_ENABLED") == true);
    ui->checkBoxCrashReporting->setChecked(Settings::getLauncherSetting("CRASH_REPORTING_ENABLED") == true);
    ui->checkBoxOpenOnGameScreen->setChecked(Settings::getLauncherSetting("OPEN_ON_GAME_SCREEN") == true);
    ui->comboBoxPlayButtonTheme->setCurrentIndex(ui->comboBoxPlayButtonTheme->findData(Settings::getLauncherSetting("PLAY_BUTTON_THEME").toString()));

    // Updates
    bool isUpdateCheckEnabled = Settings::getLauncherSetting("CHECK_FOR_UPDATES_ENABLED").toBool();
    ui->checkBoxCheckForUpdates->setChecked(isUpdateCheckEnabled);
    ui->labelRelease->setDisabled(!isUpdateCheckEnabled);
    ui->comboBoxRelease->setDisabled(!isUpdateCheckEnabled);
    ui->comboBoxRelease->setCurrentIndex(ui->comboBoxRelease->findData(Settings::getLauncherSetting("CHECK_FOR_UPDATES_RELEASE").toString()));
    ui->checkBoxAutoUpdate->setDisabled(!isUpdateCheckEnabled);
    ui->checkBoxAutoUpdate->setChecked(Settings::getLauncherSetting("AUTO_UPDATE") == true);
    ui->checkBoxBackupSaves->setDisabled(!isUpdateCheckEnabled);
    ui->checkBoxBackupSaves->setChecked(Settings::getLauncherSetting("BACKUP_SAVES") == true);
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
    Settings::setLauncherSetting("GAME_PARAM_HUMAN_PLAYER", ui->comboBoxHumanPlayer->currentData().toString());

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

    Settings::setLauncherSetting("LAUNCHER_LANGUAGE", ui->comboBoxLauncherLanguage->currentData().toString());
    Settings::setLauncherSetting("WEBSITE_INTEGRATION_ENABLED", ui->checkBoxWebsiteIntegration->isChecked() == true);
    Settings::setLauncherSetting("CRASH_REPORTING_ENABLED", ui->checkBoxCrashReporting->isChecked() == true);
    Settings::setLauncherSetting("OPEN_ON_GAME_SCREEN", ui->checkBoxOpenOnGameScreen->isChecked() == true);
    Settings::setLauncherSetting("PLAY_BUTTON_THEME", ui->comboBoxPlayButtonTheme->currentData().toString());

    // Updates
    Settings::setLauncherSetting("CHECK_FOR_UPDATES_ENABLED", ui->checkBoxCheckForUpdates->isChecked() == true);
    Settings::setLauncherSetting("CHECK_FOR_UPDATES_RELEASE", ui->comboBoxRelease->currentData().toString());
    Settings::setLauncherSetting("AUTO_UPDATE", ui->checkBoxAutoUpdate->isChecked() == true);
    Settings::setLauncherSetting("BACKUP_SAVES", ui->checkBoxBackupSaves->isChecked() == true);

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
            ui->buttonBox->button(QDialogButtonBox::Save)->setDisabled(false);
        });
    }

    // Find all QComboBox
    QList<QComboBox *> comboBoxes = ui->tabWidget->findChildren<QComboBox *>();
    for (QComboBox *comboBox : comboBoxes) {
        connect(comboBox, &QComboBox::currentIndexChanged, this, [this]() {
            this->settingHasChanged = true;
            ui->buttonBox->button(QDialogButtonBox::Save)->setDisabled(false);
        });
    }

    // Find all QLineEdit
    QList<QLineEdit *> lineEdits = ui->tabWidget->findChildren<QLineEdit *>();
    for (QLineEdit *lineEdit : lineEdits) {
        connect(lineEdit, &QLineEdit::textChanged, this, [this]() {
            this->settingHasChanged = true;
            ui->buttonBox->button(QDialogButtonBox::Save)->setDisabled(false);
        });
    }

    // Find all PopupSignalComboBox
    QList<PopupSignalComboBox *> popupComboBoxes = ui->tabWidget->findChildren<PopupSignalComboBox *>();
    for (PopupSignalComboBox *popupComboBox : popupComboBoxes) {
        connect(popupComboBox, &PopupSignalComboBox::currentIndexChanged, this, [this]() {
            this->settingHasChanged = true;
            ui->buttonBox->button(QDialogButtonBox::Save)->setDisabled(false);
        });
    }
}

void SettingsDialog::cancel()
{
    // Check if any settings have been changed
    if (settingHasChanged == true) {

        // Ask if the user is sure
        int result = QMessageBox::question(this,
                                           tr("KeeperFX Settings", "MessageBox Title"),
                                           tr("One or more settings have been changed. Are you sure you want to return without saving?", "MessageBox Text"));

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
