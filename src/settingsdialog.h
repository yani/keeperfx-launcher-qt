#pragma once

#include <QDialog>

#include "popupsignalcombobox.h"

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

private slots:
    void onOpenConfigButtonClicked();
    void onResolutionChanged(int index);
    void on_pushButtonShowLauncherParams_clicked();

private:
    Ui::SettingsDialog *ui;

    bool settingHasChanged = false;

    void loadSettings();
    void saveSettings();
    void restoreSettings();
    void cancel();

    void addSettingsChangedHandler();

    PopupSignalComboBox *popupComboBoxMonitorDisplay;
    void setupDisplayMonitorDropdown();

    QList<QWidget *> monitorNumberOverlays;
    void showMonitorNumberOverlays();
    void hideMonitorNumberOverlays();

    QStringList hiddenStartupScreens;

    void sortLanguageComboBox(QComboBox *comboBox);

    void showCustomResolutionDialog(QComboBox *sourceCombo);
    void addCustomResolution(int width, int height, QComboBox *sourceCombo);
};
