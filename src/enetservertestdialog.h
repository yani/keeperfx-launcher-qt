#pragma once

#include <QDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUdpSocket>

namespace Ui { class EnetServerTestDialog; }

class EnetServerTestDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EnetServerTestDialog(QWidget *parent = nullptr);
    ~EnetServerTestDialog();

private slots:
    void on_closeButton_clicked();
    void on_testButton_clicked();
    void handleIpReply(QNetworkReply *reply);
    void handlePingReply(QNetworkReply *reply);
    void handleUdpDatagram();

private:
    Ui::EnetServerTestDialog *ui;
    QUdpSocket *udpSocket;
    QNetworkAccessManager *networkManager;
    QString publicIp;

    void appendLog(const QString &string);
    void startUdpServer();
    void stopUdpServer();
    void sendPingRequest();
};
