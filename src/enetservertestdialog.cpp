#include "enetservertestdialog.h"
#include "kfxversion.h"
#include "ui_enetservertestdialog.h"

#include <QDateTime>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QScrollBar>

EnetServerTestDialog::EnetServerTestDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::EnetServerTestDialog)
    , udpSocket(nullptr)
    , networkManager(new QNetworkAccessManager(this))
{
    ui->setupUi(this);

    if (KfxVersion::hasFunctionality("enet_ipv6_support")) {
        ui->infoLabelIpv4->hide();
    }
}

EnetServerTestDialog::~EnetServerTestDialog()
{
    stopUdpServer();
    delete ui;
}

void EnetServerTestDialog::on_closeButton_clicked()
{
    close();
}

void EnetServerTestDialog::on_testButton_clicked()
{
    // Disable the button so we don't click it again
    ui->testButton->setDisabled(true);

    // Clear log text area
    ui->logTextArea->clear();

    // Grab public IP
    // Afterwards we run handleIpReply()
    appendLog(tr("Grabbing public IP address", "Log Message"));
    QNetworkRequest request(QUrl("https://api.ipify.org/?format=text"));
    connect(networkManager, &QNetworkAccessManager::finished, this, &EnetServerTestDialog::handleIpReply);
    networkManager->get(request);
}

void EnetServerTestDialog::handleIpReply(QNetworkReply *reply)
{
    // Disconnect the signal so we can reuse this network access manager
    disconnect(networkManager, &QNetworkAccessManager::finished, this, &EnetServerTestDialog::handleIpReply);

    // Make sure we got a response from the public IP API
    if (reply->error() != QNetworkReply::NoError) {
        appendLog(tr("Failed to retrieve public IP address", "Log Message"));
        ui->testButton->setEnabled(true);
        return;
    }

    // Get public IP from response
    publicIp = reply->readAll().trimmed();
    appendLog(tr("Public IP: %1", "Log Message").arg(publicIp));

    // Clean up
    reply->deleteLater();

    // Make sure IP address is IPv4
    if (KfxVersion::hasFunctionality("enet_ipv6_support") == false) {
        QHostAddress address(publicIp);
        if (address.protocol() != QAbstractSocket::IPv4Protocol) {
            QMessageBox::warning(this, tr("IPv6 Detected", "MessageBox Title"), tr("This tool only works with IPv4.", "MessageBox Text"));
            ui->testButton->setEnabled(true);
            return;
        }
    }

    // Start a local spoofed ENET server and start the ping procedure
    startUdpServer();
    sendPingRequest();
}

void EnetServerTestDialog::startUdpServer()
{
    appendLog(tr("Starting spoofed ENET server", "Log Message"));

    // Try to start server
    udpSocket = new QUdpSocket(this);
    if (!udpSocket->bind(QHostAddress::AnyIPv4, 5556)) {
        appendLog(tr("Failed to start ENET server", "Log Message"));
        return;
    }

    // Handle data on server
    connect(udpSocket, &QUdpSocket::readyRead, this, &EnetServerTestDialog::handleUdpDatagram);
}

void EnetServerTestDialog::stopUdpServer()
{
    if (udpSocket) {
        appendLog(tr("Stopping ENET server", "Log Message"));
        udpSocket->close();
        udpSocket->deleteLater();
        udpSocket = nullptr;
    }
}

void EnetServerTestDialog::handleUdpDatagram()
{
    while (udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        QHostAddress sender;
        quint16 senderPort;
        datagram.resize(udpSocket->pendingDatagramSize());
        udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        // This is the packet we will retrieve from the KeeperFX.net host checker tool
        // It needs to be exactly this because that's what an ENET UDP client sends to start a connection
        QByteArray expectedData = QByteArray::fromHex(
            "8fff864b82ff00010000ffff0000057800010000000000020000000000000000000013880000000200000002ec5093d400000000");

        // Check if packet matches with our ENET handshake packet
        if (datagram == expectedData) {
            appendLog(tr("Local ENET server received a valid ENET packet", "Log Message"));

            // Send some data back to let the client know we received their packet
            udpSocket->writeDatagram("OK", sender, senderPort);
        }
    }
}

void EnetServerTestDialog::sendPingRequest()
{
    appendLog(tr("Sending IP to KeeperFX.net ENET host checker tool", "Log Message"));
    QUrl url("https://keeperfx.net/workshop/tools/kfx-host-checker/ping/" + publicIp);
    QNetworkRequest request(url);
    connect(networkManager, &QNetworkAccessManager::finished, this, &EnetServerTestDialog::handlePingReply);
    networkManager->get(request);
}

void EnetServerTestDialog::handlePingReply(QNetworkReply *reply)
{
    // Disconnect the signal so we can reuse this network access manager
    disconnect(networkManager, &QNetworkAccessManager::finished, this, &EnetServerTestDialog::handlePingReply);

    // Stop ENET server
    stopUdpServer();

    // Enable test button again
    ui->testButton->setEnabled(true);

    // Make sure we got a response from the KeeperFX.net host checker tool
    if (reply->error() != QNetworkReply::NoError) {
        appendLog(tr("Failed to contact KeeperFX.net", "Log Message"));
        return;
    }

    // Get JSON response
    QJsonDocument jsonResponse = QJsonDocument::fromJson(reply->readAll());
    QJsonObject jsonObject = jsonResponse.object();

    // Show result
    if (jsonObject.contains("success") && jsonObject["success"].toBool()) {
        appendLog(tr("Your router is correctly port forwarded!", "Log Message"));
        QMessageBox::information(this, tr("Portforwarding tool", "MessageBox Title"), tr("Your router is correctly port forwarded!", "MessageBox Text"));
    } else {
        appendLog(tr("Failed to connect to local ENET server", "Log Message"));
        QMessageBox::warning(this, tr("Portforwarding tool", "MessageBox Title"), tr("Failed to connect to local ENET server.", "MessageBox Text"));
    }

    // Clean up
    reply->deleteLater();
}

void EnetServerTestDialog::appendLog(const QString &string)
{
    qDebug() << "ENET server test log:" << string;
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString timestampString = currentDateTime.toString("HH:mm:ss");
    ui->logTextArea->insertPlainText("[" + timestampString + "] " + string + "\n");

    QScrollBar *vScrollBar = ui->logTextArea->verticalScrollBar();
    if (vScrollBar) {
        vScrollBar->setValue(vScrollBar->maximum());
    }
}
