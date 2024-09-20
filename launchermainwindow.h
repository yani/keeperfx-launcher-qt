#pragma once

#include <QMainWindow>
#include <qapplication.h>
#include <qtimer.h>

QT_BEGIN_NAMESPACE
namespace Ui {
class LauncherMainWindow;
}
QT_END_NAMESPACE

class LauncherMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    LauncherMainWindow(QWidget *parent = nullptr);
    ~LauncherMainWindow();

private slots:
    void on_logFileButton_clicked();
    void on_workshopButton_clicked();
    void on_settingsButton_clicked();

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
};
