#include "apiclient.h"

#include <QMap>
#include <QImage>
#include <QEventLoop>
#include <QJsonObject>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequestFactory>

#define API_BASE_URL "https://keeperfx.net/api"

QImage ApiClient::downloadImage(QUrl url)
{
    // Initialize the network request and manager
    QNetworkRequest request(url);
    QNetworkAccessManager manager;

    // Make the network request
    QNetworkReply *reply = manager.get(request);

    // Create an event loop to wait for the request to finish
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();  // Block until the request is finished

    // Check for errors
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Failed to download image from" << url.toString() << ":" << reply->errorString();
        reply->deleteLater();
        return QImage(); // Return an empty image on failure
    }

    // Load the image from the reply data
    QByteArray imageData = reply->readAll();
    QImage image;
    if (!image.loadFromData(imageData)) {
        qWarning() << "Failed to load image from data";
        reply->deleteLater();
        return QImage(); // Return an empty image if the loading failed
    }

    // Clean up and return the image
    reply->deleteLater();
    return image;
}

QJsonDocument ApiClient::getJsonResponse(QUrl endpointPath)
{
    // Strip '/api' and slashes from the endpoint path
    QString endpointPathString = endpointPath.toString();
    if (endpointPathString.startsWith("/api")) {
        endpointPathString.remove(0, 4);
    }
    if (endpointPathString.startsWith("/")) {
        endpointPathString.remove(0, 1);
    }

    // Create full URL for logging
    QString endpointUrlString = QString(API_BASE_URL) + "/" + endpointPathString;
    qDebug() << "ApiClient: GET" << endpointUrlString;

    // Setup network manager and API
    QNetworkAccessManager manager;
    QNetworkRequestFactory api{{API_BASE_URL}};

    // Get the request
    QNetworkReply *reply = manager.get(
        api.createRequest(endpointPathString)
    );

    // Create an event loop to wait for the request to finish
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();  // Block until the request is finished

    // Check for errors
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "ApiClient [ERROR]" << endpointUrlString << "->" << reply->errorString();
        reply->deleteLater();
        return QJsonDocument();  // Return an empty QJsonDocument on error
    }

    // We retrieved something!
    qDebug() << "ApiClient:" << endpointUrlString << "-> Success";

    // Read the response and parse it as JSON
    QByteArray response = reply->readAll();
    reply->deleteLater();

    return QJsonDocument::fromJson(response);
}

QJsonObject ApiClient::getLatestStable(){

    // URL of the API endpoint
    // API endpoints can be found at: https://github.com/dkfans/keeperfx-website
    QUrl url("v1/release/stable/latest");

    // Get the JSON response
    QJsonDocument jsonDoc = ApiClient::getJsonResponse(url);
    if (jsonDoc.isObject() == false) {
        return QJsonObject();
    }

    // Convert response and return
    QJsonObject jsonObj = jsonDoc.object();
    return jsonObj["release"].toObject();
}

QJsonObject ApiClient::getLatestAlpha(){

    // URL of the API endpoint
    // API endpoints can be found at: https://github.com/dkfans/keeperfx-website
    QUrl url("v1/release/alpha/latest");

    // Get the JSON response
    QJsonDocument jsonDoc = ApiClient::getJsonResponse(url);
    if (jsonDoc.isObject() == false) {
        return QJsonObject();
    }

    // Convert response and return
    QJsonObject jsonObj = jsonDoc.object();
    return jsonObj["alpha_build"].toObject();
}

QUrl ApiClient::getDownloadUrlStable()
{
    // Get JSON object from API
    QJsonObject releaseObj = getLatestStable();
    if(releaseObj.isEmpty()){
        return QUrl();
    }

    // Get download URL
    QString downloadUrlString = releaseObj["download_url"].toString();
    qDebug() << "Stable Download URL:" << downloadUrlString;

    // Return
    return QUrl(downloadUrlString);
}

QUrl ApiClient::getDownloadUrlAlpha()
{
    // Get JSON object from API
    QJsonObject releaseObj = getLatestAlpha();
    if(releaseObj.isEmpty()){
        return QUrl();
    }

    // Get download URL
    QString downloadUrlString = releaseObj["download_url"].toString();
    qDebug() << "Alpha Download URL:" << downloadUrlString;

    // Return
    return QUrl(downloadUrlString);
}

std::optional<QMap<QString, QString>> ApiClient::getGameFileList(KfxVersion::ReleaseType type,
                                                                 QString version)
{
    // Get type as string
    QString typeString;
    if (type == KfxVersion::ReleaseType::STABLE) {
        typeString = "stable";
    } else if (type == KfxVersion::ReleaseType::ALPHA) {
        typeString = "alpha";
    } else {
        return std::nullopt;
    }

    // Get URL
    QUrl url("v1/release/" + typeString + "/" + version + "/files");

    // Get the JSON response
    QJsonDocument jsonDoc = ApiClient::getJsonResponse(url);
    if (jsonDoc.isObject() == false) {
        return std::nullopt;
    }

    // Convert to JSON object
    QJsonObject jsonObj = jsonDoc.object();

    // Make sure object is valid
    if (jsonObj["success"].toBool() != true || jsonObj["release_type"].toString() != typeString
        || jsonObj["version"].toString() != version || jsonObj.contains("files") == false) {
        return std::nullopt;
    }

    // Create path -> checksum map
    QMap<QString, QString> map;

    // Loop trough all files
    QJsonObject fileObj = jsonObj["files"].toObject();
    foreach (const QString &path, fileObj.keys()) {
        // Add to map
        map[path] = fileObj.value(path).toString();
    }

    return map;
}
