#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>

class Downloader : public QObject {
    Q_OBJECT

public:
    explicit Downloader(QObject *parent = nullptr);
    ~Downloader();

    void download(const QUrl &url, QFile *localFileOutput);

signals:
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadCompleted(bool success);

public slots:
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onReadyRead();
    void onFinished();

private:
    QNetworkAccessManager *manager;
    QNetworkReply *reply;
    QFile *localFileOutput;

    qint64 bytesWritten = 0;
    qint64 bytesTotal   = -1;
};
