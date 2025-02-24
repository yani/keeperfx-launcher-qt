#pragma once

#include "kfxversion.h"

#include <QUrl>
#include <QImage>
#include <QJsonDocument>

class ApiClient
{

public:

    static QImage downloadImage(QUrl url);
    static QJsonDocument getJsonResponse(QUrl url);

    static QJsonObject getLatestStable();
    static QJsonObject getLatestAlpha();

    static QUrl getDownloadUrlStable();
    static QUrl getDownloadUrlAlpha();

    static std::optional<QMap<QString, QString>> getGameFileList(KfxVersion::ReleaseType type, QString version);
};
