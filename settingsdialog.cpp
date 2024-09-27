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
    // ================ GAME ==================
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
}

void SettingsDialog::saveSettings()
{
    // ================ GAME ==================
    Settings::setKfxSetting("LANGUAGE", ui->comboBoxLanguage->currentData().toString());
    Settings::setLauncherSetting("CMD_OPT_NO_INTRO", ui->checkBoxSkipIntro->isChecked());
    Settings::setKfxSetting("DISABLE_SPLASH_SCREENS", ui->checkBoxDisplaySplashScreens->isChecked() == false);
    Settings::setLauncherSetting("CMD_OPT_ALEX", ui->checkBoxCheats->isChecked());
    Settings::setKfxSetting("CENSORSHIP", ui->checkBoxCensorship->isChecked());
    Settings::setKfxSetting("SCREENSHOT", ui->comboBoxScreenshots->currentData().toString());
    Settings::setLauncherSetting("CMD_OPT_FPS", ui->lineEditGameturns->text());
    Settings::setKfxSetting("COMMAND_CHAR", ui->lineEditCommandChar->text());
    Settings::setKfxSetting("DELTA_TIME", ui->checkBoxDeltaTime->isChecked());

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
    PopupSignalComboBox *popupComboBoxMonitorDisplay = new PopupSignalComboBox(parentWidget);
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

        popupComboBoxMonitorDisplay->addItem(name);
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
