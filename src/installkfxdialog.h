#pragma once

#include <QDialog>
#include <QCloseEvent>

namespace Ui {
class InstallKfxDialog;
}

class InstallKfxDialog : public QDialog {
    Q_OBJECT

public:
    explicit InstallKfxDialog(QWidget *parent = nullptr);
    ~InstallKfxDialog();

private slots:
    void on_installButton_clicked();
    void on_versionComboBox_currentIndexChanged(int index);
    void on_cancelButton_clicked();

private:
    Ui::InstallKfxDialog *ui;

    void appendLog(const QString &string);
    void updateProgress(int value);
    void setProgressMaximum(int value);
    void clearProgressBar();
    void setInstallFailed(const QString &reason);
    void closeEvent(QCloseEvent *event) override;
};
