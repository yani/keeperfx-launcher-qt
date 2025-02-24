#include "newsarticlewidget.h"
#include "ui_newsarticlewidget.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>

#include "apiclient.h"

NewsArticleWidget::NewsArticleWidget(QWidget *parent)
    : ClickableHighlightedWidget(parent)
    , ui(new Ui::NewsArticleWidget)
{
    ui->setupUi(this);
}

NewsArticleWidget::~NewsArticleWidget()
{
    delete ui;
}

void NewsArticleWidget::setTitle(QString title)
{
    ui->titleLabel->setText(title);
}

void NewsArticleWidget::setDate(QString date)
{
    ui->dateLabel->setText(date);
}

void NewsArticleWidget::setExcerpt(QString excerpt)
{
    // Use fontMetrics() to elide (shorten) the text with ellipsis
    // TODO: make the width dynamic
    QString elidedText = ui->excerptLabel->fontMetrics().elidedText(excerpt, Qt::ElideRight, 375);

    //ui->excerptLabel->setText(elidedText);

    ui->excerptLabel->setText(excerpt);
}

void NewsArticleWidget::setImage(QUrl imageUrl)
{
    // Create a dedicated temp directory
    QString cacheDir = QDir::temp().filePath("kfx-launcher-img-cache");
    QDir().mkpath(cacheDir); // Ensure the directory exists

    // Generate a shorter hash (using first 16 chars of SHA-256)
    QByteArray urlHash = QCryptographicHash::hash(imageUrl.toString().toUtf8(),
                                                  QCryptographicHash::Sha256)
                             .toHex()
                             .left(16);
    QString ext = QFileInfo(imageUrl.toString()).suffix();
    if (ext.isEmpty())
        ext = "png"; // Default to PNG if no extension found

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
