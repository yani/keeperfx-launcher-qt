#pragma once

#include <QObject>
#include <QUdpSocket>
#include <QNetworkInterface>
#include <QThreadPool>
#include <QHostInfo>
#include <QDebug>
#include <QMutex>

class EnetLanScanner : public QObject
{
    Q_OBJECT
public:
    explicit EnetLanScanner(QObject *parent = nullptr)
        : QObject(parent), udpSocket(new QUdpSocket), totalTargets(0), scannedTargets(0), responseTimeout(500) {}

    void startScan(quint16 port, int threadCount = 32, int timeout = 500)
    {
        // Variables
        responseTimeout = timeout;
        QHostAddress localAddress;
        QHostAddress subnetMask;

        // Determine local IP and subnet mask
        for (const QNetworkInterface &iface : QNetworkInterface::allInterfaces()) {
            for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol && entry.ip() != QHostAddress::LocalHost) {
                    localAddress = entry.ip();
                    subnetMask = entry.netmask();
                    break;
                }
            }
            if (!localAddress.isNull()) break;
        }

        // Make sure we found a local IP
        if (localAddress.isNull()) {
            qWarning() << "Could not determine local IP address.";
            return;
        }

        // Show some info
        qDebug() << "Scanning from" << localAddress.toString() << "with subnet" << subnetMask.toString();

        // Generate list of IPs to scan
        QList<QHostAddress> targets = getIpsInSubnet(localAddress, subnetMask);
        totalTargets = targets.size();
        scannedTargets = 0;
        QThreadPool::globalInstance()->setMaxThreadCount(threadCount);

        // Create threads for each IP
        for (QHostAddress &ip : targets) {
            QThreadPool::globalInstance()->start([this, ip, port]() { sendUdpPacket(ip, port); });
        }
    }

signals:
    void serverFound(QString ip, QString hostname);
    void scanProgress(int scanned, int total);
    void scanComplete();

private:
    QUdpSocket *udpSocket;
    int totalTargets;
    int scannedTargets;
    int responseTimeout;
    QMutex progressMutex;

    QList<QHostAddress> getIpsInSubnet(const QHostAddress &ip, const QHostAddress &mask)
    {
        QList<QHostAddress> ips;
        quint32 ipInt = ip.toIPv4Address();
        quint32 maskInt = mask.toIPv4Address();
        quint32 network = ipInt & maskInt;
        quint32 broadcast = network | ~maskInt;

        for (quint32 i = network + 1; i < broadcast; ++i) {
            ips.append(QHostAddress(i));
        }
        return ips;
    }

    void sendUdpPacket(QHostAddress target, quint16 port)
    {
        // Get IP string
        QString targetIp = target.toString();

        // Send ENET packet
        QUdpSocket socket;
        QByteArray data = QByteArray::fromHex("8fff864b82ff00010000ffff0000057800010000000000020000000000000000000013880000000200000002ec5093d400000000");
        socket.writeDatagram(data, target, port);

        // Check for response
        if (socket.waitForReadyRead(responseTimeout)) {
            qDebug() << "ENET response from:" << targetIp;

            // Emit signal with hostname
            emit serverFound(
                targetIp,
                QHostInfo::fromName(targetIp).hostName()
            );
        }

        // Update progress
        QMutexLocker locker(&progressMutex); // Use QMutexLocker for exception safety
        scannedTargets++;
        emit scanProgress(scannedTargets, totalTargets);
        if (scannedTargets == totalTargets) {
            emit scanComplete();
        }
    }
};
