#include "workshopitemwidget.h"
#include "ui_workshopitemwidget.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>

#include "apiclient.h"

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
    QString cachePath = cacheDir + "/" + urlHash + "." + ext;

    // Get pixmap
    QPixmap imagePixmap;
    if (QFile::exists(cachePath)) {
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
        imagePixmap = QPixmap::fromImage(image).scaled(ui->frame->width(), ui->frame->height(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        if (imagePixmap.isNull() == true) {
            qWarning() << "Failed to convert image to pixmap:" << imageUrl;
            return;
        }

        // Cache the image pixmap
        if (imagePixmap.save(cachePath, ext.toUpper().toLatin1().constData())) {
            qDebug() << "Image saved in cache:" << cachePath;
        } else {
            qDebug() << "Failed to cache image pixmap:" << cachePath;
        }
    }

    // Load image
    QLabel *imageLabel = new QLabel(ui->frame);
    imageLabel->setPixmap(imagePixmap);
}
