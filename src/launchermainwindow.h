#pragma once

#include "game.h"
#include "kfxversion.h"

#include <QApplication>
#include <QJsonDocument>
#include <QMainWindow>
#include <QProcess>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui {
class LauncherMainWindow;
}
QT_END_NAMESPACE

class LauncherMainWindow : public QMainWindow
{
    Q_OBJECT

signals:
    void updateFound(KfxVersion::VersionInfo versionInfo);
    void kfxNetRetrieval(QJsonDocument workshopItems, QJsonDocument latestNew);

public:
    LauncherMainWindow(QWidget *parent = nullptr);
    ~LauncherMainWindow();

private slots:
    void on_logFileButton_clicked();
    void on_workshopButton_clicked();
    void on_settingsButton_clicked();
    void on_playButton_clicked();
    void on_openFolderButton_clicked();

    void onUpdateFound(KfxVersion::VersionInfo versionInfo);
    void onGameEnded(int exitCode, QProcess::ExitStatus exitStatus);
    void onKfxNetRetrieval(QJsonDocument workshopItems, QJsonDocument latestNews);

private:
    Ui::LauncherMainWindow *ui;
    Game *game;

    void startGame(Game::StartType startType, QVariant data1 = QVariant(), QVariant data2 = QVariant(), QVariant data3 = QVariant());

    void setupPlayExtraMenu();

    QMenu *saveFilesMenu;
    void refreshSaveFilesMenu();

    QMenu *campaignMenu;
    void refreshCampaignMenu();

    void refreshInstallationAwareButtons();
    void refreshLogfileButton();

    bool isKeeperFxInstalled();
    bool askForKeeperFxInstall();

    void showLoadingSpinner();
    void hideLoadingSpinner(bool showOnlineContent);

    bool isLoadingLatestFromKfxNet = false;
    void loadLatestFromKfxNet();
    void clearLatestFromKfxNet();

    void checkForFileRemoval();
    void checkForKfxUpdate();

    void verifyBinaryCertificates();

    void refreshKfxVersionInGui();
};
