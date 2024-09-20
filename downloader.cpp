#include "downloader.h"

#include <QString>
#include <QUrl>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequestFactory>
#include <QEventLoop>
#include <QProgressBar>

void Downloader::download(QUrl url, QFile *localFileOutput,
                          std::function<void(qint64, qint64)> progressCallback,
                          std::function<void(bool)> completionCallback)
{
    // Open the file for writing, check if it opens correctly
    //if (!localFileOutput->open(QIODevice::WriteOnly)) {
    if (localFileOutput->isOpen() == false && localFileOutput->open(QIODevice::WriteOnly) == false) {
        qDebug() << "Failed to open file for writing:" << localFileOutput->errorString();
        return;
    }

    // Create a QTextStream to write to the file
    //QTextStream out(localFileOutput);

    // Create a QNetworkAccessManager on the heap to ensure it outlives the QNetworkReply
    QNetworkAccessManager *manager = new QNetworkAccessManager;

    // Create a network request with the provided URL
    QNetworkRequest request(url);

    // Start the download
    QNetworkReply *reply = manager->get(request);

    // Handle progress updates
    QObject::connect(reply, &QNetworkReply::downloadProgress, [progressCallback](qint64 bytesReceived, qint64 bytesTotal) {
        if (progressCallback) {
            progressCallback(bytesReceived, bytesTotal);
        }
    });

    // Handle data ready to be read
    QObject::connect(reply, &QNetworkReply::readyRead, [reply, localFileOutput]() {
        if (localFileOutput->isOpen()) {
            localFileOutput->write(reply->readAll());
        } else {
            qDebug() << "File is not open for writing.";
        }
    });

    // Handle request finish
    QObject::connect(reply, &QNetworkReply::finished, [reply, localFileOutput, completionCallback]() {

        if(localFileOutput->isOpen()){
            localFileOutput->close();
        }

        bool success = (reply->error() == QNetworkReply::NoError);

        if (success == false) {
            qDebug() << "Download failed:" << reply->errorString();
        }

        if (completionCallback) {
            completionCallback(success);
        }

        reply->deleteLater();
    });

    // Ensure QNetworkAccessManager lives until download completes
    QObject::connect(manager, &QNetworkAccessManager::destroyed, [manager]() {
        delete manager;
    });
}
