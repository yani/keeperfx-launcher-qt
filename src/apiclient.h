#pragma once

#include "kfxversion.h"

#include <QUrl>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>

class ApiClient
{

public:
    enum class HttpMethod { GET, POST };

    static QString getApiEndpoint();

    static QImage downloadImage(QUrl url);

    static QJsonDocument getJsonResponse(QUrl endpointPath, HttpMethod method = HttpMethod::GET, QJsonObject jsonPostObject = QJsonObject());

    static QJsonObject getLatestStable();
    static QJsonObject getLatestAlpha();

    static QUrl getDownloadUrlStable();
    static QUrl getDownloadUrlAlpha();

    static std::optional<QMap<QString, QString>> getGameFileList(KfxVersion::ReleaseType type, QString version);
};
