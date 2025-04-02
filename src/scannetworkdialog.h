#pragma once

#include "enetlanscanner.h"

#include <QDialog>

namespace Ui {
class ScanNetworkDialog;
}

class ScanNetworkDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ScanNetworkDialog(QWidget *parent = nullptr);
    ~ScanNetworkDialog();

    QString getIp();
    int getPort();

private slots:
    void on_cancelButton_clicked();
    void on_scanButton_clicked();
    void on_connectButton_clicked();
    void handleServerFound(const QString &ip, const QString &hostname);
    void handleScanProgress(int scanned, int total);
    void handleScanComplete();
    void updateConnectButton();

private:
    Ui::ScanNetworkDialog *ui;
    EnetLanScanner scanner;
    QString ip;
    int port;
};
