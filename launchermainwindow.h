#pragma once

#include "kfxversion.h"

#include <QMainWindow>
#include <QApplication>
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
    void updateFound(KfxVersion::VersionInfo versionInfo, std::optional<QMap<QString, QString>> fileMap);

public:
    LauncherMainWindow(QWidget *parent = nullptr);
    ~LauncherMainWindow();

private slots:
    void on_logFileButton_clicked();
    void on_workshopButton_clicked();
    void on_settingsButton_clicked();

    void onUpdateFound(KfxVersion::VersionInfo versionInfo, std::optional<QMap<QString, QString>> fileMap);

private:
    Ui::LauncherMainWindow *ui;

    void setupPlayExtraMenu();

    void updateAwareButtons();

    bool isKeeperFxInstalled();
    bool askForKeeperFxInstall();

    void showLoadingSpinner();
    void hideLoadingSpinner(bool showOnlineContent);

    bool isLoadingLatestFromKfxNet = false;
    void loadLatestFromKfxNet();
    void clearLatestFromKfxNet();

    void checkForFileRemoval();

    void checkForKfxUpdate();
};
