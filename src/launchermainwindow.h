#pragma once

#include "kfxversion.h"

#include <QMainWindow>
#include <QApplication>
#include <QTimer>
#include <QProcess>

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

public:
    LauncherMainWindow(QWidget *parent = nullptr);
    ~LauncherMainWindow();

private slots:
    void on_logFileButton_clicked();
    void on_workshopButton_clicked();
    void on_settingsButton_clicked();
    void on_playButton_clicked();

    void onUpdateFound(KfxVersion::VersionInfo versionInfo);
    void onGameEnded(int exitCode, QProcess::ExitStatus exitStatus);

private:
    Ui::LauncherMainWindow *ui;

    void setupPlayExtraMenu();

    void refreshPlayButtons();
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
};
