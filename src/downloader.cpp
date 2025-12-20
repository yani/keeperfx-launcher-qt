#include "downloader.h"

#include <QNetworkRequest>
#include <QDebug>

static constexpr qint64 DOWNLOAD_CHUNK_SIZE = 256 * 1024; // 256 KiB

Downloader::Downloader(QObject *parent)
    : QObject(parent),
    manager(new QNetworkAccessManager(this)),
    reply(nullptr),
    localFileOutput(nullptr)
{
}

Downloader::~Downloader()
{
    if (reply) {
        reply->abort();
        reply->deleteLater();
    }
}

void Downloader::download(const QUrl &url, QFile *file)
{
    if (reply) {
        qWarning() << "Download already in progress";
        return;
    }

    localFileOutput = file;
    bytesWritten = 0;
    bytesTotal   = -1;

    if (!localFileOutput || !localFileOutput->open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open file for writing:"
                   << (localFileOutput ? localFileOutput->errorString() : "null file");
        emit downloadCompleted(false);
        localFileOutput = nullptr;
        return;
    }

    // Create request object
    QNetworkRequest request(url);

    // Enable gzip compression
    request.setRawHeader("Accept-Encoding", "gzip");

    // Avoid Qt cache/buffering
    request.setAttribute(
        QNetworkRequest::CacheLoadControlAttribute,
        QNetworkRequest::AlwaysNetwork
    );

    // Start the download
    reply = manager->get(request);

    // Limit internal buffering
    reply->setReadBufferSize(DOWNLOAD_CHUNK_SIZE);

    // Connect signal and slots
    connect(reply, &QNetworkReply::downloadProgress, this, &Downloader::onDownloadProgress);
    connect(reply, &QNetworkReply::readyRead, this, &Downloader::onReadyRead);
    connect(reply, &QNetworkReply::finished, this, &Downloader::onFinished);
}

void Downloader::onDownloadProgress(qint64 received, qint64 total)
{
    // Qt reports *decompressed* sizes when gzip is enabled
    bytesTotal = total;
    emit downloadProgress(received, total);
}

void Downloader::onReadyRead()
{
    if (!reply || !localFileOutput || !localFileOutput->isOpen()) {
        return;
    }

    while (reply->bytesAvailable() > 0) {
        const QByteArray chunk = reply->read(DOWNLOAD_CHUNK_SIZE);
        if (chunk.isEmpty()) {
            break;
        }

        const qint64 written = localFileOutput->write(chunk);
        if (written < 0) {
            qWarning() << "Write failed:" << localFileOutput->errorString();
            reply->abort();
            return;
        }

        bytesWritten += written;

        // Emit smoother progress updates per chunk
        emit downloadProgress(bytesWritten, bytesTotal);
    }
}

void Downloader::onFinished()
{
    if (!reply) {
        return;
    }

    if (localFileOutput && localFileOutput->isOpen()) {
        localFileOutput->flush();
        localFileOutput->close();
    }

    const bool success = (reply->error() == QNetworkReply::NoError);

    if (!success) {
        qWarning() << "Download failed:" << reply->errorString();
    } else {
        // Debug: confirm gzip usage
        qDebug() << "Content-Encoding:"
                 << reply->rawHeader("Content-Encoding");
    }

    emit downloadCompleted(success);

    reply->deleteLater();
    reply = nullptr;
    localFileOutput = nullptr;
}
