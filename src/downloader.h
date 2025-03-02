#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <functional>

class Downloader : public QObject {
    Q_OBJECT

public:
    explicit Downloader(QObject *parent = nullptr);
    ~Downloader();

    void download(const QUrl &url, QFile *localFileOutput, std::function<void(qint64, qint64)> progressCallback, std::function<void(bool)> completionCallback);

private slots:
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onReadyRead();
    void onFinished();

private:
    QNetworkAccessManager *manager;
    QNetworkReply *reply;
    QFile *localFileOutput;
    std::function<void(qint64, qint64)> progressCallback;
    std::function<void(bool)> completionCallback;
};
