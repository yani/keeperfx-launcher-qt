#include "apiclient.h"

#include <QImage>
#include <QEventLoop>
#include <QJsonObject>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequestFactory>

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

QJsonDocument ApiClient::getJsonResponse(QUrl url) {

    qDebug() << "ApiClient call:" << url.toString();

    QNetworkAccessManager manager;
    QNetworkRequestFactory api{{"https://keeperfx.net/api"}};

    QNetworkReply *reply = manager.get(api.createRequest(url.toString()));

    // Create an event loop to wait for the request to finish
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();  // Block until the request is finished

    // Check for errors
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "ApiClient request failed, error:" << reply->errorString();
        reply->deleteLater();
        return QJsonDocument();  // Return an empty QJsonDocument on error
    }

    qDebug() << "ApiClient:" << url.toString() << "-> Success";

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
