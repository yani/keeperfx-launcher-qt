#pragma once

#include <QUrl>
#include <QString>
#include <QProgressBar>
#include <QJsonDocument>
#include <QFile>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

class Downloader
{
public:
    static void download(QUrl url, QFile *localFileOutput,
                         std::function<void(qint64, qint64)> progressCallback = nullptr,
                         std::function<void(bool)> completionCallback = nullptr);
};
