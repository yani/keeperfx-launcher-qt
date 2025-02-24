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

    // Generate a shorter hash (using first 16 chars of SHA-256)
    QByteArray urlHash = QCryptographicHash::hash(imageUrl.toString().toUtf8(), QCryptographicHash::Sha256).toHex().left(16);
    QString ext = QFileInfo(imageUrl.toString()).suffix();
    if (ext.isEmpty()) ext = "png"; // Default to PNG if no extension found

    QString cachePath = cacheDir + "/" + urlHash + "." + ext;

    // Get image
    QImage image;
    if (QFile::exists(cachePath)) {
        image.load(cachePath);
        qDebug() << "Image loaded from cache:" << cachePath;
    } else {
        image = ApiClient::downloadImage(imageUrl);
        if (!image.isNull()) {
            image.save(cachePath);
            qDebug() << "Image saved in cache:" << cachePath;
        }
    }

    // Make sure image exists
    if (image.isNull()) {
        return;
    }

    // Set the image
    QLabel *imageLabel = new QLabel(ui->frame);
    imageLabel->setPixmap(
        QPixmap::fromImage(image).scaled(
            ui->frame->width(),
            ui->frame->height(),
            Qt::KeepAspectRatioByExpanding,
            Qt::FastTransformation
        )
    );
}
