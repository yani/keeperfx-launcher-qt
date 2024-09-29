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

    // Reset setting has changed variable
    settingHasChanged = false;

    // Make sure this widget starts at the first tab
    // When working in Qt's UI editor you can change it to another tab by accident
    ui->tabWidget->setCurrentIndex(0);

    // Set versions in about label
    ui->labelAbout->setText(ui->labelAbout->text()
                                .replace("<kfx_version>", KfxVersion::currentVersion.string)
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
        {"Fit (default)", "FIT"},
        {"No", "false"},
        {"Stretch", "STRETCH"},
        {"Crop", "CROP"},
        {"Pixel perfect", "PIXELPERFECT"},
        {"4BY3", "4BY3"},
        {"4BY3PP", "4BY3PP"},
    };

    // Add languages
    for (auto it = resizeMoviesMap.begin(); it != resizeMoviesMap.end(); ++it) {
        ui->comboBoxResizeMovies->addItem(it.key(), it.value());
    }

    // Map: Resolutions
    QMap<QString, QString> resolutionsMap = {
        {"Match desktop", "MATCH_DESKTOP"},
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
        ui->comboBoxResolution1->addItem(it.key(), it.value());
        ui->comboBoxResolution2->addItem(it.key(), it.value());
        ui->comboBoxResolution3->addItem(it.key(), it.value());
    }

    // Map: Display mode
    QMap<QString, QString> displayModesMap = {
        {"Fullscreen", "x32"},
        {"Windowed", "w32"},
    };

    // Add display modes
    for (auto it = displayModesMap.begin(); it != displayModesMap.end(); ++it) {
        ui->comboBoxDisplayMode1->addItem(it.key(), it.value());
        ui->comboBoxDisplayMode2->addItem(it.key(), it.value());
        ui->comboBoxDisplayMode3->addItem(it.key(), it.value());
    }

    // Load the settings
    loadSettings();

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
    ui->checkBoxSkipIntro->setChecked(Settings::getLauncherSetting("CMD_OPT_NO_INTRO") == true);
    ui->checkBoxDisplaySplashScreens->setChecked(Settings::getKfxSetting("DISABLE_SPLASH_SCREENS")
                                                 == false);
    ui->checkBoxCheats->setChecked(Settings::getLauncherSetting("CMD_OPT_ALEX") == true);
    ui->checkBoxCensorship->setChecked(Settings::getKfxSetting("CENSORSHIP") == true);
    ui->comboBoxScreenshots->setCurrentIndex(
        ui->comboBoxScreenshots->findData(Settings::getKfxSetting("SCREENSHOT").toString()));
    ui->lineEditGameturns->setText(Settings::getLauncherSetting("CMD_OPT_FPS").toString());
    ui->lineEditCommandChar->setText(Settings::getKfxSetting("COMMAND_CHAR").toString());
    ui->checkBoxDeltaTime->setChecked(Settings::getKfxSetting("DELTA_TIME") == true);
    ui->checkBoxFreezeGameNoFocus->setChecked(Settings::getKfxSetting("FREEZE_GAME_ON_FOCUS_LOST")
                                              == true);

    // ============================================================================
    // ================================ GRAPHICS ==================================
    // ============================================================================

    popupComboBoxMonitorDisplay->setCurrentIndex(popupComboBoxMonitorDisplay->findData(
        Settings::getKfxSetting("DISPLAY_NUMBER").toString()));
    ui->checkBoxSmoothenVideo->setChecked(Settings::getLauncherSetting("CMD_OPT_VID_SMOOTH")
                                          == true);
    ui->comboBoxResizeMovies->setCurrentIndex(
        ui->comboBoxResizeMovies->findData(Settings::getKfxSetting("RESIZE_MOVIES").toString()));

    // Loop trough the resolutions
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

    ui->lineEditCreatureFlowerSize->setText(Settings::getKfxSetting("CREATURE_STATUS_SIZE").toString());
    ui->lineEditLineBoxSize->setText(Settings::getKfxSetting("LINE_BOX_SIZE").toString());
    ui->lineEditHandSize->setText(Settings::getKfxSetting("HAND_SIZE").toString());

    // =========================================================================
    // ================================ SOUND ==================================
    // =========================================================================


}

void SettingsDialog::saveSettings()
{

    // ========================================================================
    // ================================ GAME ==================================
    // ========================================================================

    Settings::setKfxSetting("LANGUAGE", ui->comboBoxLanguage->currentData().toString());
    Settings::setLauncherSetting("CMD_OPT_NO_INTRO", ui->checkBoxSkipIntro->isChecked());
    Settings::setKfxSetting("DISABLE_SPLASH_SCREENS",
                            ui->checkBoxDisplaySplashScreens->isChecked() == false);
    Settings::setLauncherSetting("CMD_OPT_ALEX", ui->checkBoxCheats->isChecked());
    Settings::setKfxSetting("CENSORSHIP", ui->checkBoxCensorship->isChecked());
    Settings::setKfxSetting("SCREENSHOT", ui->comboBoxScreenshots->currentData().toString());
    Settings::setLauncherSetting("CMD_OPT_FPS", ui->lineEditGameturns->text());
    Settings::setKfxSetting("COMMAND_CHAR", ui->lineEditCommandChar->text());
    Settings::setKfxSetting("DELTA_TIME", ui->checkBoxDeltaTime->isChecked());
    Settings::setKfxSetting("FREEZE_GAME_ON_FOCUS_LOST", ui->checkBoxFreezeGameNoFocus->isChecked());


    // ============================================================================
    // ================================ GRAPHICS ==================================
    // ============================================================================

    Settings::setLauncherSetting("CMD_OPT_VID_SMOOTH", ui->checkBoxSmoothenVideo->isChecked());
    Settings::setKfxSetting("DISPLAY_NUMBER", popupComboBoxMonitorDisplay->currentData().toString());
    Settings::setKfxSetting("RESIZE_MOVIES", ui->comboBoxResizeMovies->currentData().toString());

    // Save the resolutions
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
    Settings::setKfxSetting("FRONTEND_RES", resolutionString);
    Settings::setKfxSetting("CREATURE_STATUS_SIZE", ui->lineEditCreatureFlowerSize->text());
    Settings::setKfxSetting("LINE_BOX_SIZE", ui->lineEditLineBoxSize->text());
    Settings::setKfxSetting("HAND_SIZE", ui->lineEditHandSize->text());

    // =========================================================================
    // ================================ SOUND ==================================
    // =========================================================================



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
        connect(checkBox, &QCheckBox::stateChanged, this, [this]() {
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
        qDebug() <<
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
        int result = QMessageBox::question(this,
                                           "KeeperFX Settings",
                                           "One or more settings have been changed. Are you sure "
                                           "you want to return without saving?");

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
