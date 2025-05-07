#include "workshopitemwidget.h"
#include "ui_workshopitemwidget.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QPainter>

#include "apiclient.h"
#include "launcheroptions.h"

WorkshopItemWidget::WorkshopItemWidget(QWidget *parent)
    : ClickableHighlightedWidget(parent)
    , ui(new Ui::WorkshopItemWidget)
{
    ui->setupUi(this);
}

WorkshopItemWidget::~WorkshopItemWidget()
{
    delete ui;
}

void WorkshopItemWidget::setTitle(QString title)
{
    ui->titleLabel->setText(title);
}

void WorkshopItemWidget::setType(QString type)
{
    ui->typeLabel->setText(type);
}

void WorkshopItemWidget::setDate(QString date)
{
    ui->dateLabel->setText(date);
}

void WorkshopItemWidget::setAuthor(QString author)
{
    ui->authorLabel->setText(author);
}

void WorkshopItemWidget::setImage(QUrl imageUrl)
{
    // Get target size of widget
    QSize targetSize(ui->frame->width(), ui->frame->height());

    // Create a dedicated temp directory
    QString cacheDir = QDir::temp().filePath("kfx-launcher-img-cache");
    QDir().mkpath(cacheDir); // Ensure the directory exists

    // Generate a shorter hash (using first 16 chars of SHA-256) for caching
    QByteArray urlHash = QCryptographicHash::hash(imageUrl.toString().toUtf8(), QCryptographicHash::Sha256).toHex().left(16);

    // Get image file extension
    QString ext = QFileInfo(imageUrl.toString()).suffix();
    if (ext.isEmpty()) {
        ext = "png"; // Default to PNG if no extension found
    }

    // Get image cache path
    QString cachePath = cacheDir + "/" + urlHash + "_" + QString::number(targetSize.width()) + "x" + QString::number(targetSize.height()) + "." + ext;

    // Get pixmap
    QPixmap imagePixmap;
    if (QFile::exists(cachePath) && LauncherOptions::isSet("no-image-cache") == false) {
        imagePixmap.load(cachePath);
        qDebug() << "Image loaded from cache:" << cachePath;
    } else {
        // Download image
        QImage image;
        image = ApiClient::downloadImage(imageUrl);

        // Get if image was downloaded
        if (image.isNull() == true) {
            qWarning() << "Failed to download image:" << imageUrl;
            return;
        }

        // Create image pixmap
        QPixmap scaledPixmap = QPixmap::fromImage(image).scaled(targetSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        if (scaledPixmap.isNull() == true) {
            qWarning() << "Failed to convert image to pixmap:" << imageUrl;
            return;
        }

        // Create a final pixmap and center-crop it
        imagePixmap = QPixmap(targetSize);
        imagePixmap.fill(Qt::transparent); // Optional, use Qt::black or similar for background

        // Paint the image in the center
        QPainter painter(&imagePixmap);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
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
    }

    // Load image
    QLabel *imageLabel = new QLabel(ui->frame);
    imageLabel->setPixmap(imagePixmap);
}
