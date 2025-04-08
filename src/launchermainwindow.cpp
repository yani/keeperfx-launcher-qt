#include "launchermainwindow.h"
#include "./ui_launchermainwindow.h"

#include <QDesktopServices>
#include <QFile>
#include <QMessageBox>
#include <QMovie>
#include <QTimer>
#include <QUrl>
#include <QJsonArray>
#include <QShortcut>
#include <QtConcurrent/QtConcurrentRun>
#include <QMenu>
#include <QObject>
#include <QProcess>

#include "apiclient.h"
#include "campaign.h"
#include "certificate.h"
#include "clickablelabel.h"
#include "copydkfilesdialog.h"
#include "directconnectdialog.h"
#include "dkfiles.h"
#include "enetservertestdialog.h"
#include "fileremover.h"
#include "fileremoverdialog.h"
#include "game.h"
#include "installkfxdialog.h"
#include "kfxversion.h"
#include "launcheroptions.h"
#include "newsarticlewidget.h"
#include "savefile.h"
#include "scannetworkdialog.h"
#include "settings.h"
#include "settingsdialog.h"
#include "updatedialog.h"
#include "workshopitemwidget.h"

#define MAX_WORKSHOP_ITEMS_SHOWN 4
#define MAX_NEWS_ARTICLES_SHOWN 3

LauncherMainWindow::LauncherMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::LauncherMainWindow)
    , game(new Game(parent))
{
    ui->setupUi(this);

    // Connect signals and slots
    connect(this, &LauncherMainWindow::kfxNetRetrieval, this, &LauncherMainWindow::onKfxNetRetrieval);
    connect(this, &LauncherMainWindow::updateFound, this, &LauncherMainWindow::onUpdateFound);
    connect(game, &Game::gameEnded, this, &LauncherMainWindow::onGameEnded);

    // Clear placeholders
    ui->versionLabel->setText("");
    ui->spinnerLabel->setText("");
    clearLatestFromKfxNet();

    // Disable resizing and remove maximize button
    // This does not work on Wayland (for now)
    setFixedSize(size());
    setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    setWindowFlag(Qt::MSWindowsFixedSizeDialogHint);

    // Create clickable version label
    ClickableLabel *clickableVersionLabel = new ClickableLabel(this);
    clickableVersionLabel->setBaseColor(QColor("#999999"));
    clickableVersionLabel->setFont(ui->versionLabel->font());
    connect(clickableVersionLabel, &ClickableLabel::clicked, this, &LauncherMainWindow::checkForKfxUpdate);

    // Replace version label with clickable one
    ui->versionLabel->parentWidget()->layout()->replaceWidget(ui->versionLabel, clickableVersionLabel);
    ui->versionLabel->deleteLater();
    ui->versionLabel = clickableVersionLabel;

    // Load animated loading spinner GIF
    QMovie* movie = new QMovie(":/res/img/spinner.gif");
    if (movie->isValid()) {
        // Load GIF into the placeholder label
        ui->spinnerLabel->setMovie(movie);
        movie->start();
    } else {
        // Fallback to text
        ui->spinnerLabel->setText("Loading...");
    }

    // Show the loading spinner during startup
    showLoadingSpinner();

    // Create a refresh shortcut (F5) for refreshing the main panel
    connect(new QShortcut(QKeySequence(Qt::Key_F5), this), &QShortcut::activated, this, &LauncherMainWindow::loadLatestFromKfxNet);

    // Move window to the center of the main screen
    QList<QScreen *> screens = QGuiApplication::screens();
    if (screens.isEmpty() == false) {
        int screenIndex = 0;

        // Set launcher to same screen as game
        if (Settings::getLauncherSetting("OPEN_ON_GAME_SCREEN").toBool() == true) {
            screenIndex = Settings::getKfxSetting("DISPLAY_NUMBER").toInt() - 1;
            if (screenIndex >= screens.size()) {
                screenIndex = 0;
            }
        }

        // Set in middle
        QRect geometry = screens[screenIndex]->geometry();
        this->setScreen(screens[screenIndex]);
        this->move(
            // We use left() and top() here because the position is absolute and not relative to the screen
            geometry.left() + ((geometry.width() - this->width()) / 2),
            geometry.top() + ((geometry.height() - this->height()) / 2) - 50); // minus 50 to put it a bit higher
    }

    // Create a thread for loading the latest stuff from the website
    // We do this so we can already show the GUI at this point (which shows a loading spinner)
    QThread::create([this]() { loadLatestFromKfxNet(); })->start();

    // Autostart install procedure when '--install' launcher option is set
    if(LauncherOptions::isSet("install")){
        InstallKfxDialog installKfxDialog(this);
        installKfxDialog.exec();
    } else {
        // If '--install' is not forced we check if we need to install
        if (isKeeperFxInstalled() == false) {
            qDebug() << "'keeperfx.exe' seems to be missing, asking if user wants a fresh install";
            if (askForKeeperFxInstall() == true) {
                qDebug() << "User wants fresh install, opening kfx install dialog";
                InstallKfxDialog installKfxDialog(this);
                installKfxDialog.exec();
            }
        }
    }

    // Check if we need to copy over DK files
    // Only do this if keeperfx is installed
    if (isKeeperFxInstalled() == true && DkFiles::isCurrentAppDirValidDkDir() == false) {
        qDebug() << "One or more original DK files not found, opening copy dialog";
        CopyDkFilesDialog copyDkFilesWindow(this);
        copyDkFilesWindow.exec();
    }

    // Load keeperfx version if keeperfx is installed
    if (isKeeperFxInstalled()) {
        if (KfxVersion::loadCurrentVersion() == true) {
            // Refresh GUI
            refreshKfxVersionInGui();

        } else {
            // Failed to get KeeperFX version
            // Asking the user if they want to reinstall
            qDebug() << "Failed to load KeeperFX version";
            int result = QMessageBox::question(this, "KeeperFX Error",
                    tr("The launcher failed to grab the version of KeeperFX.") + " " +
                    tr("It's possible your installation is corrupted.") + "\n\n" +
                    tr("Do you want to automatically reinstall KeeperFX?")
            );

            if (result == QMessageBox::Yes) {
                // Start Automatic KeeperFX (web) installation
                qDebug() << "User wants to reinstall KeeperFX";
                InstallKfxDialog installKfxDialog(this);
                installKfxDialog.exec();

                // Try and get the version again
                if (KfxVersion::loadCurrentVersion() == true) {
                    // Refresh GUI
                    refreshKfxVersionInGui();

                } else {
                    // Still an error even after reinstalling KeeperFX
                    // We can't fix this so we'll tell the user to report it
                    QMessageBox::warning(this, "KeeperFX Error",
                        tr("The launcher failed to grab the version of KeeperFX.") + " " +
                        tr("Please report this error to the KeeperFX team.")
                                         );
                }
            }
        }
    }

    // Load the extra menu for the button next to the play button
    setupPlayExtraMenu();

    // Handle buttons that should be aware of the current installation
    // This will enable/disable specific buttons whether the logfile and the KFX binary exist
    refreshInstallationAwareButtons();
    refreshLogfileButton();

    // Start checking periodically if the logfile exists
    // This continious checking is not required but it's an interesting little gimmick
    QTimer *logfileButtonRefreshTimer = new QTimer();
    connect(logfileButtonRefreshTimer, &QTimer::timeout, this, &LauncherMainWindow::refreshLogfileButton);
    logfileButtonRefreshTimer->start(2500);

    // Verify the binaries against known certificates
    verifyBinaryCertificates();

    // Check if there are any files that should be removed
    checkForFileRemoval();

    // Check for updates
    checkForKfxUpdate();
}

LauncherMainWindow::~LauncherMainWindow()
{
    delete ui;
}

void LauncherMainWindow::showLoadingSpinner()
{
    // Hide the elements in the main panel view
    ui->KfxNewsList->hide();
    ui->KfxWorkshopItemList->hide();
    ui->latestWorkshopItemsLabel->hide();
    ui->latestNewsLabel->hide();
    ui->LogoWidget->hide();

    // Show the spinner
    // Because all the rest is hidden this will nicely show in the middle
    ui->spinnerLabel->show();
}

void LauncherMainWindow::hideLoadingSpinner(bool showOnlineContent)
{
    // Hide the spinner
    ui->spinnerLabel->hide();

    if(showOnlineContent){

        // Show workshop items if there are some
        if(ui->KfxWorkshopItemList->children().length() > 0){
            ui->latestWorkshopItemsLabel->show();
            ui->KfxWorkshopItemList->show();
        }

        // Show news articles if there are some
        if(ui->KfxNewsList->children().length() > 0){
            ui->latestNewsLabel->show();
            ui->KfxNewsList->show();
        }
    }
    else
    {
        ui->LogoWidget->show();
    }
}

void LauncherMainWindow::setupPlayExtraMenu()
{
    QMenu *menu = new QMenu(this);

    // Play map action
    menu->addAction(tr("Play map"),
                    [this]() {
                        // Handle play map logic here
                        qDebug() << "Play map selected!";
                    })
        ->setDisabled(true); // TODO: disabled until implemented

    // Play campaign action
    if (KfxVersion::hasFunctionality("start_campaign_directly") == true) {
        this->campaignMenu = menu->addMenu(tr("Play campaign"));
        menu->addMenu(campaignMenu);
        refreshCampaignMenu();
    }

    // Add 'Load game'
    // We store this menu in the main window so we can reload the saves when the game ends
    if (KfxVersion::hasFunctionality("load_save_directly") == true) {
        this->saveFilesMenu = menu->addMenu(tr("Load save game"));
        menu->addMenu(saveFilesMenu);
        refreshSaveFilesMenu();
    }

    // Direct connect (MP) action
    if (KfxVersion::hasFunctionality("direct_enet_connect") == true) {
        menu->addAction(tr("Direct connect (MP)"),
            [this]() {
                qDebug() << "Direct connect (MP) selected!";
                // Open the dialog
                DirectConnectDialog dialog(this);
                if (dialog.exec() == QDialog::Accepted) {
                    startGame(Game::StartType::DIRECT_CONNECT, dialog.getIp(), dialog.getPort());
                }
        });
    }

    // Scan local network (MP)
    menu->addAction(tr("Scan local network (MP)"),
        [this]() {
            qDebug() << "Scan local network (MP) selected!";
            // Open the scan dialog
            ScanNetworkDialog dialog(this);
            if (dialog.exec() == QDialog::Accepted) {
                // Start the game
                startGame(Game::StartType::DIRECT_CONNECT, dialog.getIp(), dialog.getPort());
            }
        }
    );

    // Scan local network (MP)
    menu->addAction(tr("Test internet lobby (MP)"), [this]() {
        qDebug() << "Test internet lobby (MP) selected!";
        // Open the scan dialog
        EnetServerTestDialog dialog(this);
        dialog.exec();
    });

    // Run packetsave action
    menu->addAction(tr("Run packetfile"),
                    [this]() {
                        // Handle run packetsave logic here
                        qDebug() << "Run packetsave selected!";
                    })
        ->setDisabled(true); // TODO: disabled until implemented

    // Heavylog toggle
    QFile heavyLogBin(QCoreApplication::applicationDirPath() + "/keeperfx_hvlog.exe");
    if (heavyLogBin.exists()) {
        // Create menu item
        QAction *heavyLogAction = menu->addAction(tr("Heavylog"));
        heavyLogAction->setCheckable(true);
        // Store if heavylog is enabled
        bool heavyLogEnabled = Settings::getLauncherSetting("GAME_HEAVY_LOG_ENABLED").toBool();
        // Set toggle state of menu item
        heavyLogAction->setChecked(heavyLogEnabled);
        // Update play button
        ui->playButton->setText(heavyLogEnabled ? "  " + tr("Play (hvlog)") : tr("Play"));
        // Connect heavylog toggle
        connect(heavyLogAction, &QAction::triggered, this, [this, heavyLogAction]() {
            bool enabled = heavyLogAction->isChecked();
            ui->playButton->setText(enabled ? "  " + tr("Play (hvlog)") : tr("Play"));
            // Update settings to remember this
            Settings::setLauncherSetting("GAME_HEAVY_LOG_ENABLED", enabled);
        });
    }

    // Attach the menu to the button
    ui->playExtraButton->setMenu(menu);
}

void LauncherMainWindow::refreshSaveFilesMenu()
{
    if (KfxVersion::hasFunctionality("load_save_directly") == false) {
        return;
    }

    // Clear any old loaded saves
    this->saveFilesMenu->clear();

    // Add saves to 'Load game'
    QList<SaveFile *> saveFileList = SaveFile::getAll();
    if (saveFileList.empty()) {
        this->saveFilesMenu->setDisabled(true);
    } else {
        this->saveFilesMenu->setDisabled(false);
        for (SaveFile *saveFile : saveFileList) {
            this->saveFilesMenu->addAction(saveFile->toString(), [this, saveFile]() {
                // Handle loading the save file
                qDebug() << "Loading save file:" << saveFile;
                // TODO: startGame(Game::StartType::LOAD_SAVE, saveFile->saveName);
            });
        }
    }
}

void LauncherMainWindow::refreshCampaignMenu()
{
    if (KfxVersion::hasFunctionality("start_campaign_directly") == false) {
        return;
    }

    // Clear any old loaded campaigns in the list
    this->campaignMenu->clear();

    // Add campaigns to 'Start campaign'
    QList<Campaign *> campaignList = Campaign::getAll();
    if (campaignList.empty()) {
        this->campaignMenu->setDisabled(true);
    } else {
        this->campaignMenu->setDisabled(false);
        for (Campaign *campaign : campaignList) {
            this->campaignMenu->addAction(campaign->toString(), [this, campaign]() {
                // Start campaign
                qDebug() << "Starting campaign:" << campaign->toString();
                startGame(Game::StartType::CAMPAIGN, campaign->campaignShortName);
            });
        }
    }
}

void LauncherMainWindow::refreshInstallationAwareButtons() {
    bool isKfxInstalled = isKeeperFxInstalled();
    ui->settingsButton->setDisabled(isKfxInstalled == false);
    ui->playButton->setDisabled(isKfxInstalled == false);
    ui->playExtraButton->setDisabled(isKfxInstalled == false);
}

void LauncherMainWindow::refreshLogfileButton() {
    QFile logFile(QCoreApplication::applicationDirPath() + "/keeperfx.log");
    ui->logFileButton->setDisabled(logFile.exists() == false);
}

void LauncherMainWindow::on_logFileButton_clicked()
{
    // Use default text editor to open keeperfx.log file
    QString logFilePath = QCoreApplication::applicationDirPath() + "/keeperfx.log";
    QFile logFile(logFilePath);
    if (logFile.exists()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(logFilePath));
    } else {
        qWarning() << "File does not exist: " << logFilePath;
    }
}

void LauncherMainWindow::on_workshopButton_clicked()
{
    // Use default browser to open the workshop URL
    QUrl url("https://keeperfx.net/workshop");
    QDesktopServices::openUrl(url);
}

bool LauncherMainWindow::isKeeperFxInstalled()
{
    // Check for 'keeperfx.exe' file in app directory
    QFile keeperFxBin (QCoreApplication::applicationDirPath() + "/keeperfx.exe");
    return (keeperFxBin.exists());
}

bool LauncherMainWindow::askForKeeperFxInstall()
{
    // Ask if user wants to install KeeperFX
    int result = QMessageBox::question(this, "KeeperFX",
        tr("The launcher is unable to find 'keeperfx.exe'.") + "\n\n" + tr("Do you want to download and install KeeperFX?")
    );

    // Check if user declines
    if(result != QMessageBox::Yes){

        // Ask if they are sure
        result = QMessageBox::question(this, "Confirmation",
                                       tr("Are you sure?") + "\n\n" + tr("You will be unable to play KeeperFX."));
        if(result == QMessageBox::Yes){

            // User does not want to install KeeperFX
            return false;
        }
    }

    // User wants to install KeeperFX
    return true;
}

void LauncherMainWindow::on_settingsButton_clicked() {

    // Remember
    QString oldReleaseVersion = Settings::getLauncherSetting("CHECK_FOR_UPDATES_RELEASE").toString();
    bool oldUpdateCheckEnabled = Settings::getLauncherSetting("CHECK_FOR_UPDATES_ENABLED").toBool();
    QString oldLauncherLanguage = Settings::getLauncherSetting("LAUNCHER_LANGUAGE").toString();
    bool websiteIntegration = Settings::getLauncherSetting("WEBSITE_INTEGRATION_ENABLED").toBool();

    // Open settings dialog
    SettingsDialog settingsDialog(this);
    settingsDialog.exec();

    // Check if launcher language has changed
    if(oldLauncherLanguage != Settings::getLauncherSetting("LAUNCHER_LANGUAGE").toString()){

        // Ask user if they want to restart their launcher
        qDebug() << "Launcher language has changed so asking for launcher restart";
        int result = QMessageBox::question(this, "Language has changed",
            tr("The launcher has to restart to change its language.") + " " +
                                               tr("Do you want to do that now?")
            );
        if (result == QMessageBox::Yes) {

            qDebug() << "Restarting launcher to change the language";

            // Hide this launcher's window and pipe the exit code from the new launcher process to the exit of this one
            // We do this because we want to allow users to keep a handle on the original process
            this->hide();
            QCoreApplication::exit(
                QProcess::execute(QCoreApplication::applicationFilePath(), QCoreApplication::arguments())
            );
            return;
        }
    }

    // Check for updates when settings are closed
    if(Settings::getLauncherSetting("CHECK_FOR_UPDATES_ENABLED").toBool() == true && (
            // Check for updates was disabled and has been enabled now
            oldUpdateCheckEnabled == false ||
            // Version has changed
            oldReleaseVersion != Settings::getLauncherSetting("CHECK_FOR_UPDATES_RELEASE").toString()
    )){
        qDebug() << "Settings regarding updates have been enabled or changed so asking for update";
        checkForKfxUpdate();
    }

    // Website integration
    if (websiteIntegration != Settings::getLauncherSetting("WEBSITE_INTEGRATION_ENABLED").toBool()) {
        showLoadingSpinner(); // This also hides everything
        loadLatestFromKfxNet();
    }
}

void LauncherMainWindow::clearLatestFromKfxNet()
{
    // Delete all news articles and labels within the parent widget.
    // We don't delete all childeren in the main window because the layout is part of it too.
    // We also don't use "delete later" because we check if the list is empty afterwards.

    for (NewsArticleWidget *widget : ui->KfxNewsList->findChildren<NewsArticleWidget*>()) {
        delete widget;
    }
    for (WorkshopItemWidget *widget : ui->KfxWorkshopItemList->findChildren<WorkshopItemWidget*>()) {
        delete widget;
    }
    for (QLabel *label : ui->KfxNewsList->findChildren<QLabel*>()) {
        delete label;
    }
    for (QLabel *label : ui->KfxWorkshopItemList->findChildren<QLabel*>()) {
        delete label;
    }
}

void LauncherMainWindow::loadLatestFromKfxNet()
{
    // Check if website integration is disabled
    if (Settings::getLauncherSetting("WEBSITE_INTEGRATION_ENABLED") == false) {
        hideLoadingSpinner(false);
        return;
    }

    // Make sure we're not already loading
    if (isLoadingLatestFromKfxNet) {
        return;
    }

    // Remember that we are loading
    isLoadingLatestFromKfxNet = true;

    // Make sure the loading spinner is shown
    showLoadingSpinner();

    // Clear the existing data in the lists
    clearLatestFromKfxNet();

    // API call: Latest Workshop Items
    auto fetchWorkshopItems = QtConcurrent::run([this]() {
        return ApiClient::getJsonResponse(QUrl("/v1/workshop/latest"));
    });

    // API call: Latest KfxNews
    auto fetchLatestNews = QtConcurrent::run([this]() {
        return ApiClient::getJsonResponse(QUrl("/v1/news/latest"));
    });

    // Wait for both threads to finish
    fetchWorkshopItems.waitForFinished();
    fetchLatestNews.waitForFinished();

    // Process the results
    QJsonDocument workshopItems = fetchWorkshopItems.result();
    QJsonDocument latestNews = fetchLatestNews.result();

    // Emit a signal to pass the data to the main thread
    emit kfxNetRetrieval(workshopItems, latestNews);
}

void LauncherMainWindow::onKfxNetRetrieval(QJsonDocument workshopItems, QJsonDocument latestNews)
{
    if (workshopItems.isEmpty() == false) {
        QJsonObject workshopItemsObj = workshopItems.object();
        QJsonArray workshopItemsArray = workshopItemsObj["workshop_items"].toArray();

        int count = 0;
        for (const QJsonValue &workshopItemValue : workshopItemsArray) {
            // Variables
            QJsonObject workshopItem = workshopItemValue.toObject();
            QJsonObject submitterObj = workshopItem["submitter"].toObject();

            // Create the widget
            WorkshopItemWidget *workshopItemWidget = new WorkshopItemWidget(ui->KfxWorkshopItemList);
            workshopItemWidget->setTitle(workshopItem["name"].toString());
            workshopItemWidget->setType(workshopItem["category"].toString());
            workshopItemWidget->setDate(workshopItem["created_timestamp"].toString());
            workshopItemWidget->setAuthor(submitterObj["username"].toString());
            workshopItemWidget->setTargetUrl(workshopItem["url"].toString());

            // Set the size of the widget
            // TODO: make this dynamic (as it doesn't look good with a bigger window)
            workshopItemWidget->setMaximumWidth(250);
            workshopItemWidget->setMaximumHeight(110);

            if (workshopItem["thumbnail"].isNull() == false) {
                workshopItemWidget->setImage(QUrl(workshopItem["thumbnail"].toString()));
            } else {
                // The API returns a default image for items without one so we can just pass it
                workshopItemWidget->setImage(QUrl(workshopItem["image"].toString()));
            }

            // Add the widget to the list
            ui->KfxWorkshopItemList->layout()->addWidget(workshopItemWidget);

            // Only allow the max amount of items
            if (++count >= MAX_WORKSHOP_ITEMS_SHOWN) {
                break;
            }
        }
    }

    if (latestNews.isEmpty() == false) {
        QJsonObject newsArticlesObj = latestNews.object();
        QJsonArray newsArticlesArray = newsArticlesObj["articles"].toArray();

        int count = 0;
        for (const QJsonValue &newsArticleValue : newsArticlesArray) {
            // Get the json object
            QJsonObject newsArticle = newsArticleValue.toObject();

            // Create the widget
            NewsArticleWidget *newsArticleWidget = new NewsArticleWidget(ui->KfxNewsList);
            newsArticleWidget->setTitle(newsArticle["title"].toString());
            newsArticleWidget->setDate(newsArticle["created_timestamp"].toString());
            newsArticleWidget->setTargetUrl(newsArticle["url"].toString());

            if (newsArticle["excerpt"].isNull() == false) {
                newsArticleWidget->setExcerpt(newsArticle["excerpt"].toString());
            } else {
                newsArticleWidget->setExcerpt("");
            }

            // Set the size of the widget
            // TODO: make this dynamic (as it doesn't look good with a bigger window)
            newsArticleWidget->setMaximumWidth(510);
            newsArticleWidget->setMaximumHeight(110);

            // The API returns a default image for items without one so we can just pass it
            newsArticleWidget->setImage(QUrl(newsArticle["image"].toString()));

            // Add the widget to the list
            ui->KfxNewsList->layout()->addWidget(newsArticleWidget);

            // Only allow the max amount of items
            if (++count >= MAX_NEWS_ARTICLES_SHOWN) {
                break;
            }
        }
    }

    this->hideLoadingSpinner(true);
    isLoadingLatestFromKfxNet = false;
}

void LauncherMainWindow::checkForFileRemoval()
{
    // Remove 'save/deleteme.txt'
    // We remove it manually because this will almost always be present after an install (or update)
    QFile saveDeleteMeFile(QCoreApplication::applicationDirPath() + "/save/deleteme.txt");
    if (saveDeleteMeFile.exists()) {
        saveDeleteMeFile.remove();
    }

    // Check if 'files-to-remove.txt' file exists
    QFile fileRemovalFile(QCoreApplication::applicationDirPath() + "/files-to-remove.txt");
    if (fileRemovalFile.exists()) {

        // Get files to remove based on KfxVersion
        QStringList filesToRemove = FileRemover::processFile(fileRemovalFile,
                                                             KfxVersion::currentVersion.version);

        // If there are files that should be removed
        if(filesToRemove.length() > 0){
            qDebug() << "Files found that should be removed:" << filesToRemove;

            // Show file removal dialog
            FileRemoverDialog fileRemoverDialog(this, filesToRemove);
            fileRemoverDialog.exec();
        }
    }
}

void LauncherMainWindow::onUpdateFound(KfxVersion::VersionInfo versionInfo)
{
    // Start updater
    UpdateDialog updateDialog(this, versionInfo);
    updateDialog.exec();

    // Reload current version
    if (KfxVersion::loadCurrentVersion() == true) {
        refreshKfxVersionInGui();
    }

    // Get the executable path of a possible updated launcher
#ifdef Q_OS_WINDOWS
    QString newAppBinString(QCoreApplication::applicationDirPath()
                            + "/keeperfx-launcher-qt-new.exe");
#else
    QString newAppBinString(QCoreApplication::applicationDirPath() + "/keeperfx-launcher-qt-new");
#endif

    // Check if new launcher update exists
    QFile newAppBin(newAppBinString);
    if (newAppBin.exists()) {
        qDebug() << "New launcher found:" << newAppBinString;
        qDebug() << "Starting new launcher";
        // Start the new launcher
        // This needs to be detached because we are going the remove the current running launcher
        QProcess::startDetached(newAppBinString, QCoreApplication::arguments());
        QApplication::quit();
        return;
    }

    // Verify the binaries against known certificates
    verifyBinaryCertificates();

    // Check if there are any files that should be removed
    checkForFileRemoval();

    // Refresh the installation-aware buttons
    // It's possible an update is done that fixes the installation and adds the binary again
    refreshInstallationAwareButtons();
}

void LauncherMainWindow::checkForKfxUpdate()
{
    // Check for updates if they are enabled
    if (Settings::getLauncherSetting("CHECK_FOR_UPDATES_ENABLED") != true) {
        qInfo() << "Checking for updates is disabled";
        return;
    }

    // Only update from stable and alpha
    if (KfxVersion::currentVersion.type != KfxVersion::ReleaseType::STABLE &&
        KfxVersion::currentVersion.type != KfxVersion::ReleaseType::ALPHA) {
        qDebug() << "Not updating because we are not on stable or alpha version";
        return;
    }

    // Spawn a thread
    // We don't want any slow internet connections block our main thread
    QThread::create([this]() {

        // Get release type
        QString typeString = Settings::getLauncherSetting("CHECK_FOR_UPDATES_RELEASE").toString();
        KfxVersion::ReleaseType type = KfxVersion::getReleaseTypefromString(typeString);

        // Only update to stable and alpha
        if (type != KfxVersion::ReleaseType::STABLE && type != KfxVersion::ReleaseType::ALPHA) {
            qDebug() << "Invalid auto update release type:" << typeString;
            return;
        }

        // Get latest version for this release type
        auto latestVersionInfo = KfxVersion::getLatestVersion(type);
        if (latestVersionInfo) {

            // Check if type of release is different or version is newer
            if (type != KfxVersion::currentVersion.type
                || KfxVersion::isNewerVersion(latestVersionInfo->version,
                                              KfxVersion::currentVersion.version)) {
                qDebug() << "Update found:" << latestVersionInfo->version;

                // Emit signal for update
                emit this->updateFound(latestVersionInfo.value());

            } else {
                qDebug() << "No updates found.";
            }
        }
    })->start();
}

void LauncherMainWindow::verifyBinaryCertificates()
{
    // Check if we need to skip verification
    if (LauncherOptions::isSet("skip-verify") == true) {
        qDebug() << "Skipping certificate file verification (skip-verify)";
        return;
    }

    // List of files to check
    QStringList filesToCheck = {
        "keeperfx.exe", "keeperfx_hvlog.exe",
        // "keeperfx-launcher-qt.exe",
    };

    QStringList failedFiles;

    // Loop through and load each certificate
    for (const QString &filePath : filesToCheck) {
        // Load file
        QFile file(QApplication::applicationDirPath() + "/" + filePath);

        // Make sure file exists
        if (file.exists() == false) {
            continue;
        }

        // Verify
        if (Certificate::verify(file) == false) {
            failedFiles.append(filePath);
        }
    }

    // If any files have been failed to verify
    if (failedFiles.empty() == false) {

        // Create file list
        QString fileListString;
        for (const QString &filePath : failedFiles) {
            fileListString.append(filePath + "\n");
        }

        // Show messagebox alerting the user
        QMessageBox::warning(this, "KeeperFX Verification Error",
            tr("The launcher failed to verify the signature of") + ":\n\n" +
            fileListString + "\n" +
            tr("It is highly suggested to only use official KeeperFX files.")
        );
    }
}

void LauncherMainWindow::startGame(Game::StartType startType, QVariant data1, QVariant data2, QVariant data3)
{
    // Disable the play buttons
    ui->playButton->setDisabled(true);
    ui->playExtraButton->setDisabled(true);

    // Start the game
    bool startStatus = game->start(startType, data1, data2, data3);

    // Make sure game is started
    if (startStatus == false) {
        qDebug() << "Game failed to start";

        // Refresh the installation-aware and logfile buttons
        refreshInstallationAwareButtons();
        refreshLogfileButton();

        // Get the error
        QString errorString = game->getErrorString();

        // Show messagebox alerting the user
        if (errorString.isEmpty() == false) {
            qDebug() << "Game start error:" << errorString;
            QMessageBox::warning(this, "KeeperFX", tr("Failed to start KeeperFX.") + "\n\n" + tr("Error") + ":\n" + errorString);
        } else {
            QMessageBox::warning(this, "KeeperFX", tr("Failed to start KeeperFX.") + " " + tr("Unknown error."));
        }
    }
}

void LauncherMainWindow::on_playButton_clicked()
{
    startGame(Game::StartType::NORMAL);
}

void LauncherMainWindow::onGameEnded(int exitCode, QProcess::ExitStatus exitStatus)
{
    refreshInstallationAwareButtons();
    refreshLogfileButton();
    refreshSaveFilesMenu();

    // Not really required but good to occasionally refresh
    refreshCampaignMenu();
}

void LauncherMainWindow::refreshKfxVersionInGui()
{
    qInfo() << "KeeperFX version:" << KfxVersion::currentVersion.fullString;
    ui->versionLabel->setText("v" + KfxVersion::currentVersion.fullString);
    this->setWindowTitle("KeeperFX Launcher - v" + KfxVersion::currentVersion.fullString);
}

void LauncherMainWindow::on_openFolderButton_clicked()
{
    // Use default file browser to open Application Folder
    QUrl url = QUrl::fromLocalFile(QCoreApplication::applicationDirPath() + QDir::separator());
    QDesktopServices::openUrl(url);
}
