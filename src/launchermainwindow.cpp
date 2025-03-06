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
#include "certificate.h"
#include "copydkfilesdialog.h"
#include "crc32.h"
#include "dkfiles.h"
#include "fileremover.h"
#include "fileremoverdialog.h"
#include "installkfxdialog.h"
#include "kfxversion.h"
#include "launcheroptions.h"
#include "newsarticlewidget.h"
#include "savefile.h"
#include "settings.h"
#include "settingsdialog.h"
#include "updatedialog.h"
#include "updater.h"
#include "workshopitemwidget.h"

#define MAX_WORKSHOP_ITEMS_SHOWN 4
#define MAX_NEWS_ARTICLES_SHOWN 3

LauncherMainWindow::LauncherMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::LauncherMainWindow)
{
    ui->setupUi(this);

    // Clear placeholders
    ui->versionLabel->setText("");
    ui->spinnerLabel->setText("");
    clearLatestFromKfxNet();

    // Disable resizing and remove maximize button
    // This does not work on Wayland (for now)
    setFixedSize(size());
    setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    setWindowFlag(Qt::MSWindowsFixedSizeDialogHint);

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
    // TODO: allow user to set a launcher startup monitor
    QList<QScreen *> screens = QGuiApplication::screens();
    if (screens.isEmpty() == false) {
        QRect geometry = screens[0]->geometry();
        this->setScreen(screens[0]);
        this->move(
            // We use left() and top() here because the position is absolute and not relative to the screen
            geometry.left() + ((geometry.width() - this->width()) / 2),
            geometry.top() + ((geometry.height() - this->height()) / 2) - 50); // minus 50 to put it a bit higher
    }

    // Create a thread for loading the latest stuff from the website
    // We do this so we can already show the GUI at this point (which shows a loading spinner)
    // The function within the thread will invoke updating the GUI, so it's thread safe
    QThread::create([this]() { loadLatestFromKfxNet(); })->start();

    // Autostart install procedure when '--install' launcher option is set
    if(LauncherOptions::isSet("install")){
        InstallKfxDialog installKfxDialog(this);
        installKfxDialog.exec();
    }

    // Check if KeeperFX is installed
    if (isKeeperFxInstalled() == false) {
        // Ask if user wants to install KeeperFX
        qDebug() << "'keeperfx.exe' seems to be missing, asking if user wants a fresh install";
        if (askForKeeperFxInstall() == true) {
            // Open automatic KeeperFX (web) installer
            qDebug() << "User wants fresh install, opening kfx install dialog";
            InstallKfxDialog installKfxDialog(this);
            installKfxDialog.exec();
        }
    }

    // Check if we need to copy over DK files
    // Only do this if keeperfx is installed
    if (isKeeperFxInstalled() == true && DkFiles::isCurrentAppDirValidDkDir() == false) {
        // Open copy DK files dialog
        qDebug() << "One or more original DK files not found, opening copy dialog";
        CopyDkFilesDialog copyDkFilesWindow(this);
        copyDkFilesWindow.exec();
    }

    // Load keeperfx version if keeperfx is installed
    if (isKeeperFxInstalled()) {
        if (KfxVersion::loadCurrentVersion() == true) {
            // Version successfully loaded
            // Add the version to the the GUI
            qInfo() << "KeeperFX version:" << KfxVersion::currentVersion.fullString;
            ui->versionLabel->setText("v" + KfxVersion::currentVersion.fullString);
            this->setWindowTitle("KeeperFX Launcher - v" + KfxVersion::currentVersion.fullString);

        } else {
            // Failed to get KeeperFX version
            // Asking the user if they want to reinstall
            qDebug() << "Failed to load keeperfx version";
            int result
                = QMessageBox::question(this,
                                        "KeeperFX Error",
                                        "The launcher failed to grab the version of KeeperFX. It's "
                                        "possible your installation is corrupted."
                                        "\n\nDo you want to automatically reinstall KeeperFX?");

            if (result == QMessageBox::Yes) {
                // Start Automatic KeeperFX (web) installation
                qDebug() << "User wants to reinstall KeeperFX";
                InstallKfxDialog installKfxDialog(this);
                installKfxDialog.exec();

                // Try and get the version again
                if (KfxVersion::loadCurrentVersion() == true) {
                    // Version successfully loaded
                    // Add the version to the the GUI
                    qInfo() << "KeeperFX version:" << KfxVersion::currentVersion.fullString;
                    ui->versionLabel->setText("v" + KfxVersion::currentVersion.fullString);
                    this->setWindowTitle("KeeperFX Launcher - v"
                                         + KfxVersion::currentVersion.fullString);

                } else {
                    // Still an error even after reinstalling KeeperFX
                    // We can't fix this so we'll tell the user to report it
                    QMessageBox::warning(this,
                                         "KeeperFX Error",
                                         "The launcher failed to grab the version of KeeperFX. "
                                         "Please report this error to the KeeperFX team.");
                }
            }
        }
    }

    // Load settings
    Settings::load();

    // Load the extra menu for the button next to the play button
    setupPlayExtraMenu();

    // Handle buttons that should be aware of the current installation
    // This will enable/disable specific buttons whether the logfile and the KFX binary exist
    updateAwareButtons();

    // Start checking the installation aware buttons periodically
    // This continious checking is not required but it's an interesting little gimmick
    QTimer *buttonAwareTimer = new QTimer();
    connect(buttonAwareTimer, &QTimer::timeout, this, &LauncherMainWindow::updateAwareButtons);
    buttonAwareTimer->start(2500);

    // Verify the binaries against known certificates
    verifyBinaryCertificates();

    // Check if there are any files that should be removed
    checkForFileRemoval();

    // Check for updates
    connect(this, &LauncherMainWindow::updateFound, this, &LauncherMainWindow::onUpdateFound);
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
    menu->addAction(tr("Play map"), [this]() {
        // Handle play map logic here
        qDebug() << "Play map selected!";
    });

    // Play campaign action
    menu->addAction(tr("Play campaign"), [this]() {
        // Handle play campaign logic here
        qDebug() << "Play campaign selected!";
    });

    // Add 'Load game'
    QMenu *saveFilesMenu = menu->addMenu(tr("Load game"));
    menu->addMenu(saveFilesMenu);

    // Check if the save file dir exists
    QDir saveFileDir(QApplication::applicationDirPath() + "/save");
    if (saveFileDir.exists() == false) {
        saveFilesMenu->setDisabled(true);
    } else {
        // Get the save files
        QStringList saveFileFilter;
        saveFileFilter << "fx1g*.sav";
        QStringList saveFiles = saveFileDir.entryList(saveFileFilter, QDir::Files);
        if (!saveFiles.isEmpty()) {

            // Add save files to the submenu
            for (const QString &saveFileFilename : saveFiles) {

                // Get save file
                SaveFile *saveFile = new SaveFile(saveFileDir.absoluteFilePath(saveFileFilename));
                if(saveFile->isValid()){
                    // Add to menu
                    saveFilesMenu->addAction(saveFile->toString(), [this, saveFile]() {

                        // Handle loading the save file
                        qDebug() << "Loading save file:" << saveFile;
                    });
                }
            }
        } else {
            // Disable the submenu if no save files are found
            saveFilesMenu->setDisabled(true);
        }
    }

    // Direct connect (MP) action
    menu->addAction(tr("Direct connect (MP)"), [this]() {
        // Handle direct connect logic here
        qDebug() << "Direct connect (MP) selected!";
    });

    // Run packetsave action
    menu->addAction(tr("Run packetfile"), [this]() {
        // Handle run packetsave logic here
        qDebug() << "Run packetsave selected!";
    });

    // Run heavylog action
    QFile heavyLogBin(QCoreApplication::applicationDirPath() + "/keeperfx_hvlog.exe");
    menu->addAction(tr("Run heavylog"), [this]() {
        // Handle run heavylog logic here
        qDebug() << "Run heavylog selected!";
    })->setDisabled(
        heavyLogBin.exists() == false
    );

    // Attach the menu to the button
    ui->playExtraButton->setMenu(menu);

    // TODO: disabled until implemented
    menu->setDisabled(true); // TODO: disabled until implemented
}

void LauncherMainWindow::updateAwareButtons() {

    // Handle play buttons
    ui->playButton->setDisabled(isKeeperFxInstalled() == false);
    ui->playExtraButton->setDisabled(isKeeperFxInstalled() == false);

    // Handle logfile button
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
        "The launcher is unable to find 'keeperfx.exe'."
        "\n\nDo you want to download and install KeeperFX?"
    );

    // Check if user declines
    if(result != QMessageBox::Yes){

        // Ask if they are sure
        result = QMessageBox::question(this, "Confirmation",
                                       "Are you sure?\n\nYou will be unable to play KeeperFX.");
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

    // Open settings dialog
    SettingsDialog settingsDialog(this);
    settingsDialog.exec();

    // Check for updates when settings are closed and update release version has changed
    if (oldReleaseVersion != Settings::getLauncherSetting("CHECK_FOR_UPDATES_RELEASE").toString()) {
        qDebug() << "Game release version changed so asking for update";
        checkForKfxUpdate();
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

    // TODO: we should first load all the remote images that need to be shown
    //       and only then should we invoke adding the new widgets
    //       that way the main thread is locked the least

    // Make sure this runs on the main thread
    // We do it like this so we can add the widgets on the main thread
    // even if we are running from a seperate thread
    QMetaObject::invokeMethod(this, [this, workshopItems, latestNews]() {

        if(workshopItems.isEmpty() == false){

            QJsonObject workshopItemsObj = workshopItems.object();
            QJsonArray workshopItemsArray = workshopItemsObj["workshop_items"].toArray();

            int count = 0;
            for(const QJsonValue &workshopItemValue: workshopItemsArray){

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

                if(workshopItem["thumbnail"].isNull() == false){
                    workshopItemWidget->setImage(QUrl(workshopItem["thumbnail"].toString()));
                } else {
                    // The API returns a default image for items without one so we can just pass it
                    workshopItemWidget->setImage(QUrl(workshopItem["image"].toString()));
                }

                // Add the widget to the list
                ui->KfxWorkshopItemList->layout()->addWidget(workshopItemWidget);

                // Only allow the max amount of items
                if(++count >= MAX_WORKSHOP_ITEMS_SHOWN){
                    break;
                }
            }
        }

        if(latestNews.isEmpty() == false){

            QJsonObject newsArticlesObj = latestNews.object();
            QJsonArray newsArticlesArray = newsArticlesObj["articles"].toArray();

            int count = 0;
            for(const QJsonValue &newsArticleValue: newsArticlesArray){

                // Get the json object
                QJsonObject newsArticle = newsArticleValue.toObject();

                // Create the widget
                NewsArticleWidget *newsArticleWidget = new NewsArticleWidget(ui->KfxNewsList);
                newsArticleWidget->setTitle(newsArticle["title"].toString());
                newsArticleWidget->setDate(newsArticle["created_timestamp"].toString());
                newsArticleWidget->setTargetUrl(newsArticle["url"].toString());

                if(newsArticle["excerpt"].isNull() == false){
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
                if(++count >= MAX_NEWS_ARTICLES_SHOWN){
                    break;
                }
            }
        }

        this->hideLoadingSpinner(true);
        isLoadingLatestFromKfxNet = false;
    });
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
        // Version successfully loaded
        // Add the version to the the GUI
        qInfo() << "KeeperFX version:" << KfxVersion::currentVersion.fullString;
        ui->versionLabel->setText("v" + KfxVersion::currentVersion.fullString);
        this->setWindowTitle("KeeperFX Launcher - v" + KfxVersion::currentVersion.fullString);
    }

    // Verify the binaries against known certificates
    verifyBinaryCertificates();

    // Check if there are any files that should be removed
    checkForFileRemoval();
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
        "keeperfx.exe",
        "keeperfx_hvlog.exe",
        "launcher.exe",
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
        QMessageBox::warning(this,
             "KeeperFX Verification Error",
             "The launcher failed to verify the signature of:\n\n"
                 + fileListString +
                 "\nIt is highly suggested to only use official KeeperFX files.");
    }
}

void LauncherMainWindow::on_playButton_clicked()
{
    QProcess *process = new QProcess();
    QString keeperfxBin = QApplication::applicationDirPath() + "/keeperfx.exe";

#ifdef Q_OS_WINDOWS
    QStringList params;
    process->start(keeperfxBin, params);
#else
    qDebug() << "try start";

    qDebug() << "Running as user:" << qgetenv("USER");

    // Make sure no weird Qt environment shenanigans
    //QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    //env.insert("PATH", "/usr/bin:/bin:/usr/local/bin");
    //process->setProcessEnvironment(env);

    if (!QFile::exists("/usr/bin/wine")) {
        qWarning() << "Error: /usr/bin/wine does not exist!";
        return;
    } else if (!QFileInfo("/usr/bin/wine").isExecutable()) {
        qWarning() << "Error: /usr/bin/wine is not executable!";
        return;
    }


    QStringList params;
    params << keeperfxBin;
    process->start("/usr/bin/wine", params);
#endif

    // Wait for process to start and check errors
    if (!process->waitForStarted()) {
        qDebug() << "Error: Process failed to start. Error:" << process->errorString();
        return;
    }
}
