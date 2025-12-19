#include "downloader.h"
#include <QDebug>

Downloader::Downloader(QObject *parent) : QObject(parent), manager(new QNetworkAccessManager(this)), reply(nullptr), localFileOutput(nullptr) {}

Downloader::~Downloader() {
    if (reply) {
        reply->deleteLater();
    }
    if (manager) {
        manager->deleteLater();
    }
    if (localFileOutput) {
        delete localFileOutput;
    }
}

void Downloader::download(const QUrl &url, QFile *localFileOutput) {
    this->localFileOutput = localFileOutput;
    if (!localFileOutput->open(QIODevice::WriteOnly)) {
        qDebug() << "Failed to open file for writing:" << localFileOutput->errorString();
        emit downloadCompleted(false);
        delete localFileOutput;
        localFileOutput = nullptr;
        return;
    }

    QNetworkRequest request(url);
    reply = manager->get(request);

    connect(reply, &QNetworkReply::downloadProgress, this, &Downloader::onDownloadProgress);
    connect(reply, &QNetworkReply::readyRead, this, &Downloader::onReadyRead);
    connect(reply, &QNetworkReply::finished, this, &Downloader::onFinished);
}

void Downloader::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    emit downloadProgress(bytesReceived, bytesTotal);
}

void Downloader::onReadyRead() {
    if (localFileOutput->isOpen()) {
        localFileOutput->write(reply->readAll());
    } else {
        qWarning() << "File is not open for writing.";
    }
}

void Downloader::onFinished() {

    if (localFileOutput->isOpen()) {
        localFileOutput->close();
    }

    bool success = (reply->error() == QNetworkReply::NoError);

    if (!success) {
        qWarning() << "Download failed:" << reply->errorString();
    }

    emit downloadCompleted(success);

    // Disconnect connected signals
    disconnect(reply, &QNetworkReply::downloadProgress, this, &Downloader::onDownloadProgress);
    disconnect(reply, &QNetworkReply::readyRead, this, &Downloader::onReadyRead);
    disconnect(reply, &QNetworkReply::finished, this, &Downloader::onFinished);

    // Cleanup
    reply->deleteLater();
    reply = nullptr;
}
