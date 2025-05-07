#pragma once

#include "launcheroptions.h"

#include <QDir>
#include <QEventLoop>
#include <QImage>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QPainter>
#include <QProcess>
#include <QString>
#include <QVariant>

class ImageHelper
{
public:
    static QImage download(QUrl url)
    {
        // Initialize the network request and manager
        QNetworkRequest request(url);
        QNetworkAccessManager manager;

        // Make the network request
        QNetworkReply *reply = manager.get(request);

        // Create an event loop to wait for the request to finish
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec(); // Block until the request is finished

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

    static QPixmap getOnlineScaledPixmap(QUrl url, QSize targetSize)
    {
        QPixmap imagePixmap;

        // Create a dedicated temp directory
        QString cacheDir = QDir::temp().filePath("kfx-launcher-img-cache");
        QDir().mkpath(cacheDir); // Ensure the directory exists

        // Generate a shorter hash (using first 16 chars of SHA-256) for caching
        QByteArray urlHash = QCryptographicHash::hash(url.toString().toUtf8(), QCryptographicHash::Sha256).toHex().left(16);

        // Get image file extension
        QString ext = QFileInfo(url.toString()).suffix();
        if (ext.isEmpty()) {
            ext = "png"; // Default to PNG if no extension found
        }

        // Get image cache path
        QString cachePath = cacheDir + "/" + urlHash + "_" + QString::number(targetSize.width()) + "x" + QString::number(targetSize.height()) + "." + ext;

        // Check if image is cached
        if (QFile::exists(cachePath) && LauncherOptions::isSet("no-image-cache") == false) {
            imagePixmap.load(cachePath);
            qDebug() << "Image loaded from cache:" << cachePath;
            return imagePixmap;
        }

        // Download image
        QImage image;
        image = download(url);

        // Get if image was downloaded
        if (image.isNull() == true) {
            qWarning() << "Failed to download image:" << url;
            return QPixmap();
        }

        // Create image pixmap
        QPixmap scaledPixmap = QPixmap::fromImage(image).scaled(targetSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        if (scaledPixmap.isNull() == true) {
            qWarning() << "Failed to convert image to pixmap:" << url;
            return QPixmap();
        }

        // Create a final pixmap and center-crop it
        imagePixmap = QPixmap(targetSize);
        imagePixmap.fill(Qt::transparent); // Optional, use Qt::black or similar for background

        // Paint the image in the center
        QPainter painter(&imagePixmap);
        //painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.drawPixmap((targetSize.width() - scaledPixmap.width()) / 2, (targetSize.height() - scaledPixmap.height()) / 2, scaledPixmap);
        painter.end();

        // Cache the image pixmap
        if (LauncherOptions::isSet("no-image-cache") == false) {
            if (imagePixmap.save(cachePath, ext.toUpper().toLatin1().constData())) {
                qDebug() << "Image saved in cache:" << cachePath;
            } else {
                qDebug() << "Failed to cache image pixmap:" << cachePath;
            }
        }

        // Return the image pixmap
        return imagePixmap;
    }
};
